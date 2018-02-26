/*
 * Copyright 2023 Matthieu Bouron <matthieu.bouron@gmail.com>
 * Copyright 2023 Nope Forge
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <jni.h>
#include <pthread.h>
#include <unistd.h>

#include <android/log.h>

#include <libavformat/avformat.h>
#include <libavcodec/jni.h>
#include <libavcodec/mediacodec.h>
#include <libavutil/log.h>
#include <libavutil/time.h>
#include <libavutil/avstring.h>

#include <nopemd.h>
#include <assert.h>

#define LOG_TAG "NopeMD"

static void av_android_log(void *arg, int level, const char *fmt, va_list vl);

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    av_jni_set_java_vm(vm, NULL);
    av_log_set_callback(av_android_log);

    return JNI_VERSION_1_6;
}

static void av_android_log(void *arg, int level, const char *fmt, va_list vl)
{
    int android_log_level = ANDROID_LOG_ERROR;

    struct {
        int av_log_level;
        int android_log_level;
    } static const mapping[] = {
            {AV_LOG_TRACE,   ANDROID_LOG_VERBOSE},
            {AV_LOG_VERBOSE, ANDROID_LOG_VERBOSE},
            {AV_LOG_DEBUG,   ANDROID_LOG_DEBUG},
            {AV_LOG_INFO,    ANDROID_LOG_INFO},
            {AV_LOG_WARNING, ANDROID_LOG_WARN},
            {AV_LOG_ERROR,   ANDROID_LOG_ERROR},
    };

    for (int i = 0; i < sizeof(mapping)/sizeof(*mapping); i++) {
        if (level == mapping[i].av_log_level) {
            android_log_level = mapping[i].android_log_level;
            break;
        }
    }
    __android_log_vprint(android_log_level, "AV", fmt, vl);
}

static void nmd_android_log(void *arg, int level, const char *filename, int ln, const char *fn, const char *fmt, va_list vl)
{
    int i;
    int android_log_level = ANDROID_LOG_ERROR;

    struct {
        int nmd_log_level;
        int android_log_level;
    } static const mapping[] = {
            { NMD_LOG_VERBOSE, ANDROID_LOG_VERBOSE },
            { NMD_LOG_DEBUG, ANDROID_LOG_DEBUG },
            { NMD_LOG_INFO, ANDROID_LOG_INFO },
            { NMD_LOG_WARNING, ANDROID_LOG_WARN },
            { NMD_LOG_ERROR, ANDROID_LOG_ERROR },
    };

    for (i = 0; i < sizeof(mapping)/sizeof(*mapping); i++) {
        if (level == mapping[i].nmd_log_level) {
            android_log_level = mapping[i].android_log_level;
            break;
        }
    }

    __android_log_vprint(android_log_level, LOG_TAG, fmt, vl);
}

JNIEXPORT void JNICALL
Java_org_nopeforge_NopeMD_1android_NopeMD_nativeDecodeAllFramesToSurface(JNIEnv *env,
                                                                             jclass type,
                                                                             jstring filename_,
                                                                             jint live,
                                                                             jobject surface)
{
    jobject surface_ = (*env)->NewGlobalRef(env, surface);
    const char *filename = (*env)->GetStringUTFChars(env, filename_, 0);

    int nb_frames = 0;
    int nb_frames2 = 0;
    double t = av_gettime();
    struct nmd_frame *frame = NULL;

    struct nmd_ctx *s = nmd_create(filename);
    if (!s)
        return;

    nmd_set_option(s, "max_nb_packets", 1);
    nmd_set_option(s, "max_nb_frames", 1);
    nmd_set_option(s, "max_nb_sink", 1);
    nmd_set_option(s, "auto_hwaccel", 1);
    nmd_set_option(s, "opaque", &surface_);
    nmd_set_log_callback(s, NULL, nmd_android_log);

    while ((frame = nmd_get_next_frame(s)) != NULL) {
        nb_frames++;
        if (frame->pix_fmt == NMD_PIXFMT_MEDIACODEC) {
            nmd_mc_frame_render_and_releasep(&frame);
        } else {
            nmd_frame_releasep(&frame);
        }

        if (nb_frames == 1) {
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Got first frame in %g s", av_gettime() - t) ;
            t = av_gettime();
        }

        if (live && nb_frames >= 600)
            break;
    }
    t = av_gettime() - t;
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Got %d frames in %g: %g fps", nb_frames, t, nb_frames * 1000000 / t) ;

    if (live)
        goto done;

    for (int i = 0; i < nb_frames; i++) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Ask frame at %g", i * 1/60.0);
        frame = nmd_get_frame(s, i * 1/60.0);
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Got frame %p at %g", frame, i * 1/60.0);
        if (frame && frame->pix_fmt == NMD_PIXFMT_MEDIACODEC) {
            nmd_mc_frame_render_and_releasep(&frame);
            nb_frames2++;
        } else {
            nmd_frame_releasep(&frame);
        }
        usleep(1000000 * 1/60.0);
    }
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Got %d/%d frames", nb_frames2, nb_frames);

    static const double timestamps[] = {
            0.0, 2.0, 1.0, 3.0, 2.0, 4.0, 3.0, 5.0, 4.0, 6.0, 5.0,
            0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0,
            0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
            0.0, 3.0, 3.1, 3.2, 3.3, 3.4, 3.0, 3.1, 3.2, 3.3, 3.4,
            6.5, 6.4, 6.3, 6.2, 6.1, 6.0, 6.5, 6.0, 5.5, 5.0, 4.5,
    };

    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < sizeof(timestamps) / sizeof(*timestamps); i++) {
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Ask for frame at %g", timestamps[i]);
            frame = nmd_get_frame(s, timestamps[i]);
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Got frame at %g", timestamps[i]);
            if (frame && frame->pix_fmt == NMD_PIXFMT_MEDIACODEC) {
                nmd_mc_frame_render_and_releasep(&frame);
            } else {
                nmd_frame_releasep(&frame);
            }
            if (j && (i == 0 || i == 10 || i == 20))
                sleep(10);
        }
    }

done:
    nmd_freep(&s);

    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Done");

    (*env)->ReleaseStringUTFChars(env, filename_, filename);
    (*env)->DeleteGlobalRef(env, surface_);
}

static char *str_concat(char *dst, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    char buf[4096];
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);

    va_end(ap);

    if (n >= 4095) {
        return NULL;
    }

    int size = strlen(dst) + n + 1;
    char *ret = realloc(dst, size);
    if (!ret) {
        free(dst);
        return NULL;
    }

    ret = strcat(ret, buf);
    return ret;
}

static int str_write(const char *filename, const char *data)
{
    if (!data)
        return -1;

    FILE *fp = fopen(filename, "w");
    if (!fp)
        return -1;

    fwrite(data, sizeof(*data), strlen(data), fp);

    fclose(fp);
    return 0;
}

struct NopeMD_stat {
    int eof;
    int64_t nb_frames;
    double *frame_timestamps;
    double *frame_decode_times;
};

JNIEXPORT void JNICALL
Java_org_nopeforge_nmd_1android_NopeMD_nativeMultipleDecodesToSurfaces(JNIEnv *env,
                                                                              jclass type,
                                                                              jstring model_,
                                                                              jstring filename_,
                                                                              jobjectArray surfaces,
                                                                              jint nb_surfaces,
                                                                              jint nb_frames,
                                                                              jstring output_path_)
{
    const char *model = (*env)->GetStringUTFChars(env, model_, 0);
    const char *filename = (*env)->GetStringUTFChars(env, filename_, 0);
    const char *output_path = (*env)->GetStringUTFChars(env, output_path_, 0);

    jobject *surface_references = malloc(nb_surfaces * sizeof(jobject));
    for (int i = 0; i < nb_surfaces; i++) {
        jobject surface = (*env)->GetObjectArrayElement(env, surfaces, i);
        surface_references[i] = (*env)->NewGlobalRef(env, surface);
        (*env)->DeleteLocalRef(env, surface);
    }

    struct nmd_ctx **players = calloc(nb_surfaces, sizeof(struct NopeMD_ctx *));
    for (int i = 0; i < nb_surfaces; i++) {
        struct nmd_ctx *ctx = nmd_create(filename);
        nmd_set_option(ctx, "max_nb_packets", 10);
        nmd_set_option(ctx, "max_nb_frames", 1);
        nmd_set_option(ctx, "max_nb_sink", 1);
        nmd_set_option(ctx, "auto_hwaccel", 1);
        nmd_set_option(ctx, "opaque", &surface_references[i]);
        nmd_set_log_callback(ctx, NULL, nmd_android_log);
        nmd_start(ctx);
        nmd_seek(ctx, i * 20);
        players[i] = ctx;
    }

    struct NopeMD_stat *player_stats = calloc(nb_surfaces, sizeof(*player_stats));
    if (!player_stats)
        return;

    for (int i = 0; i < nb_surfaces; i++) {
        struct NopeMD_stat *player_stat = &player_stats[i];
        player_stat->frame_timestamps = calloc(nb_frames, sizeof(*player_stat->frame_timestamps));
        player_stat->frame_decode_times = calloc(nb_frames, sizeof(*player_stat->frame_decode_times));
        if (!player_stat->frame_timestamps || !player_stat->frame_decode_times)
            return;
    }

    int64_t global_timer = av_gettime();
    double *global_frame_timestamps = calloc(nb_frames, sizeof(*global_frame_timestamps));
    double *global_frame_decode_times = calloc(nb_frames, sizeof(*global_frame_decode_times));
    if (!global_frame_timestamps || !global_frame_decode_times)
        return;

    int cur_frames = 0;
    int got_first_frames = 0;
    while (cur_frames < nb_frames) {
        int64_t local_timer = av_gettime();
        if (!got_first_frames) {
            int start = 1;
            for (int i = 0; i < nb_surfaces; i++) {
                if (player_stats[i].nb_frames <= 0) {
                    start = 0;
                    break;
                }
            }
            if (start) {
                got_first_frames = 1;
                global_timer = av_gettime();
            }
        }


        if (cur_frames % 100 == 0) {
            for (int i = 0; i < nb_surfaces; i++) {
                nmd_seek(players[i], i * 10);
            }
        }


        for (int i = 0; i < nb_surfaces; i++) {
            int64_t timer = av_gettime();
            struct NopeMD_stat *stat = &player_stats[i];
            if (stat->eof)
                continue;

            struct nmd_frame *frame = nmd_get_next_frame(players[i]);
            if (frame) {
                float ts = frame->ts;
                if (frame->pix_fmt == NMD_PIXFMT_MEDIACODEC) {
                    nmd_mc_frame_render_and_releasep(&frame);
                } else {
                    nmd_frame_releasep(&frame);
                }

                stat->frame_timestamps[stat->nb_frames] = ts;
                stat->frame_decode_times[stat->nb_frames] = (av_gettime() - timer) / (double)1000000;
                stat->nb_frames++;
            } else {
                stat->eof = 1;
            }
        }
        global_frame_decode_times[cur_frames] = (av_gettime() - local_timer) / (double)1000000;
        cur_frames++;

        int eof = 1;
        for (int i = 0; i < nb_surfaces; i++) {
            struct NopeMD_stat *stat = &player_stats[i];
            if (!stat->eof) {
                eof = 0;
                break;
            }
        }

        if (eof)
            break;
    }

    global_timer = av_gettime() - global_timer;

    nb_frames = -1;
    for (int i = 0; i < nb_surfaces; i++) {
        struct NopeMD_stat *stat = &player_stats[i];
        if (nb_frames < 0) {
            nb_frames = stat->nb_frames;
        }

        if (stat->nb_frames != nb_frames) {
            __android_log_print(ANDROID_LOG_ERROR, "NopeMD", "decoded frame count does not match");
        }
    }

    int nb_active_decoders = 0;
    for (int i = 0; i < nb_surfaces; i++) {
        struct NopeMD_stat *stat = &player_stats[i];
        if (stat->nb_frames > 0)
            nb_active_decoders++;
    }

    char *json_data = calloc(1, sizeof(*json_data));
    json_data = str_concat(json_data, "{\n");
    json_data = str_concat(json_data, "   \"model\": \"%s\",\n", model);
    json_data = str_concat(json_data, "   \"filename\": \"%s\",\n", filename);
    json_data = str_concat(json_data, "   \"nb_decoders\": %d,\n", nb_surfaces);
    json_data = str_concat(json_data, "   \"nb_active_decoders\": %d,\n", nb_active_decoders);
    json_data = str_concat(json_data, "   \"nb_frames\": %d,\n", nb_frames);
    json_data = str_concat(json_data, "   \"fps\": %f,\n", nb_frames * 1000000LL / (double)global_timer);
    json_data = str_concat(json_data, "   \"decode_times\": [\n");
    for (int i = 0; i < nb_frames; i++) {
    json_data = str_concat(json_data, "       %f%s", global_frame_decode_times[i], i < nb_frames - 1 ? "," : "");
    }
    json_data = str_concat(json_data, "   ]\n");
    json_data = str_concat(json_data, "}\n\n");

    __android_log_print(ANDROID_LOG_ERROR, "NopeMD", "Writing: %sto %s", json_data, output_path);
    str_write(output_path, json_data);
    free(json_data);

    free(global_frame_timestamps);
    free(global_frame_decode_times);

    for (int i = 0; i < nb_surfaces; i++) {
        struct NopeMD_stat *stat = &player_stats[i];
        if (stat) {
            free(stat->frame_timestamps);
            free(stat->frame_decode_times);
        }
    }
    free(player_stats);

    for (int i = 0; i < nb_surfaces; i ++) {
        if (players[i])
            nmd_freep(&players[i]);
    }
    free(players);

    for (int i = 0; i < nb_surfaces; i++) {
        if (surface_references[i])
            (*env)->DeleteGlobalRef(env, surface_references[i]);
    }
    free(surface_references);

    (*env)->ReleaseStringUTFChars(env, filename_, filename);
    (*env)->ReleaseStringUTFChars(env, model_, model);
    (*env)->ReleaseStringUTFChars(env, output_path_, output_path);
}

JNIEXPORT void JNICALL
Java_org_nopeforge_nmd_1android_NopeMD_nativeSeekAndDecodeToSurfaces(JNIEnv *env,
                                                                        jclass type,
                                                                        jstring model_,
                                                                        jstring filename_,
                                                                        jobject surface_,
                                                                        jstring output_path_) {


    const char *model = (*env)->GetStringUTFChars(env, model_, 0);
    const char *filename = (*env)->GetStringUTFChars(env, filename_, 0);
    jobject surface = (*env)->NewGlobalRef(env, surface_);
    const char *output_path = (*env)->GetStringUTFChars(env, output_path_, 0);

    struct nmd_ctx *player = nmd_create(filename);
    nmd_set_option(player, "max_nb_packets", 1);
    nmd_set_option(player, "max_nb_frames", 1);
    nmd_set_option(player, "max_nb_sink", 1);
    nmd_set_option(player, "auto_hwaccel", 1);
    nmd_set_option(player, "opaque", &surface);
    nmd_set_log_callback(player, NULL, nmd_android_log);
    nmd_start(player);

    double seek_positions[512] = {0};
    int nb_seek_positions = sizeof(seek_positions) / sizeof(*seek_positions);

    for (int i = 0; i < nb_seek_positions; i++) {
        seek_positions[i] = i / (double)60;
    }

    struct NopeMD_stat *player_stat = calloc(1, sizeof(*player_stat));
    if (!player_stat)
        return;

    player_stat->frame_timestamps = calloc(nb_seek_positions, sizeof(*player_stat->frame_timestamps));
    player_stat->frame_decode_times = calloc(nb_seek_positions, sizeof(*player_stat->frame_decode_times));
    if (!player_stat->frame_timestamps || !player_stat->frame_decode_times)
        return;

    int frame_count = 0;
    double average_seek_time = 0.0;
    for (int i = 0; i < nb_seek_positions; i++) {
        int64_t timer = av_gettime();
        nmd_seek(player, seek_positions[i]);
        struct nmd_frame *frame = nmd_get_frame(player, seek_positions[i]);
        if (frame) {
            float ts = frame->ts;
            frame_count++;
            if (frame->pix_fmt == NMD_PIXFMT_MEDIACODEC) {
                nmd_mc_frame_render_and_releasep(&frame);
            } else {
                nmd_frame_releasep(&frame);
            }
            player_stat->frame_decode_times[i] = (av_gettime() - timer) / 1000000.0;
            player_stat->frame_timestamps[i] = ts;
            average_seek_time += player_stat->frame_decode_times[i];
        } else {
            player_stat->frame_decode_times[i] = -1.0;
        }
    }
    average_seek_time /= frame_count;

    char *json_data = calloc(1, sizeof(*json_data));
    json_data = str_concat(json_data, "{\n");
    json_data = str_concat(json_data, "   \"model\": \"%s\",\n", model);
    json_data = str_concat(json_data, "   \"filename\": \"%s\",\n", filename);
    json_data = str_concat(json_data, "   \"nb_frames\": %d,\n", frame_count);
    json_data = str_concat(json_data, "   \"avg_seek_time\": %f,\n", average_seek_time);
    json_data = str_concat(json_data, "   \"seek_times\": [\n");
    for (int i = 0; i < frame_count; i++)
    json_data = str_concat(json_data, "       %f%s\n", player_stat->frame_decode_times[i], i < (frame_count - 1) ? "," : "");
    json_data = str_concat(json_data, "   ]\n");
    json_data = str_concat(json_data, "}\n\n");

    __android_log_print(ANDROID_LOG_ERROR, "NopeMD", "Writing: %sto %s", json_data, output_path);
    str_write(output_path, json_data);
    free(json_data);

    if (player_stat) {
        free(player_stat->frame_timestamps);
        free(player_stat->frame_decode_times);
    }
    free(player_stat);

    nmd_freep(&player);

    if (surface)
        (*env)->DeleteGlobalRef(env, surface);

    (*env)->ReleaseStringUTFChars(env, filename_, filename);
    (*env)->ReleaseStringUTFChars(env, model_, model);
    (*env)->ReleaseStringUTFChars(env, output_path_, output_path);
}

JNIEXPORT void JNICALL
Java_org_nopeforge_nmd_1android_NopeMD_nativeRandomSeekAndDecodeToSurface(JNIEnv *env,
                                                                                 jclass type,
                                                                                 jstring filename_,
                                                                                 jobject surface)
{
    jobject surface_ = (*env)->NewGlobalRef(env, surface);
    const char *filename = (*env)->GetStringUTFChars(env, filename_, 0);

    int nb_frames = 0;
    int nb_frames2 = 0;
    double t = av_gettime();

    double timestamp = 0.0;
    static const double timestamps[] = {
            0.0, 2.0, 1.0, 3.0, 2.0, 4.0, 3.0, 5.0, 4.0, 6.0, 5.0,
            0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0,
            0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
            0.0, 3.0, 3.1, 3.2, 3.3, 3.4, 3.0, 3.1, 3.2, 3.3, 3.4,
            6.5, 6.4, 6.3, 6.2, 6.1, 6.0, 6.5, 6.0, 5.5, 5.0, 4.5,
    };

    struct nmd_ctx *s = nmd_create(filename);
    if (!s)
        return;

    nmd_set_option(s, "max_nb_packets", 1);
    nmd_set_option(s, "max_nb_frames", 1);
    nmd_set_option(s, "max_nb_sink", 1);
    nmd_set_option(s, "sw_pix_fmt", NMD_PIXFMT_RGBA);
    nmd_set_option(s, "opaque", &surface_);
    nmd_set_log_callback(s, NULL, nmd_android_log);

    struct nmd_frame *frame = NULL;
    while ((frame = nmd_get_next_frame(s)) != NULL) {
        nb_frames++;
        if (frame->pix_fmt == NMD_PIXFMT_MEDIACODEC) {
            nmd_mc_frame_render_and_releasep(&frame);
        } else {
            nmd_frame_releasep(&frame);
        }

        if (nb_frames == 1)
            t = av_gettime();

        if (nb_frames >= 600)
            break;

    }
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Got %d frames in %f", nb_frames, av_gettime() - t);

    for (int i = 0; i < nb_frames; i++) {
        frame = nmd_get_frame(s, timestamp);
        if (frame && frame->pix_fmt == NMD_PIXFMT_MEDIACODEC) {
            nmd_mc_frame_render_and_releasep(&frame);
            nb_frames2++;
        } else {
            nmd_frame_releasep(&frame);
        }

        timestamp += 1/60.0f;
        usleep(16666);
    }
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Got %d/%d frames", nb_frames2, nb_frames);

    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < sizeof(timestamps) / sizeof(*timestamps); i++) {
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Ask for frame at %f", timestamps[i]);
            frame = nmd_get_frame(s, timestamps[i]);
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Got frame at %f", timestamps[i]);
            if (frame && frame->pix_fmt == NMD_PIXFMT_MEDIACODEC) {
                nmd_mc_frame_render_and_releasep(&frame);
            } else {
                nmd_frame_releasep(&frame);
            }
            if (j && (i == 0 || i == 10 || i == 20))
                sleep(10);
        }
    }

    nmd_freep(&s);
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Done");

    (*env)->ReleaseStringUTFChars(env, filename_, filename);
    (*env)->DeleteGlobalRef(env, surface_);
}

JNIEXPORT void JNICALL
Java_org_nopeforge_nmd_1android_NopeMD_nativeAudioDecode(JNIEnv *env, jclass type,
                                                                jstring filename_) {
    const char *filename = (*env)->GetStringUTFChars(env, filename_, 0);

    struct nmd_ctx *s = nmd_create(filename);
    if (!s)
        return;

    nmd_set_option(s, "avselect", NMD_SELECT_AUDIO);
    nmd_set_option(s, "audio_texture", 0);
    nmd_set_option(s, "max_nb_packets", 1);
    nmd_set_option(s, "max_nb_frames", 1);
    nmd_set_option(s, "max_nb_sink", 1);
    nmd_set_log_callback(s, NULL, nmd_android_log);

    double t = 0;
    int nb_frames = 0;
    struct nmd_frame *frame;
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ">>");
    while ((frame = nmd_get_next_frame(s)) != NULL) {
        nb_frames++;
        nmd_frame_releasep(&frame);

        if (nb_frames == 1)
            t = av_gettime();
    }
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Got %d frames in %f", nb_frames, av_gettime() - t);

    nmd_freep(&s);

    (*env)->ReleaseStringUTFChars(env, filename_, filename);
}
