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

package org.nopeforge.nmd_android;

import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import androidx.annotation.NonNull;

import java.io.IOException;
import java.nio.ByteBuffer;

public class MediaCodecVideoDecoder2 extends Thread {
    private final String filename;
    private final Surface surface;
    private MediaExtractor extractor;
    private MediaFormat format;
    private MediaCodec codec;
    private int nbFrames;
    private int frames;
    private long timer;
    private boolean stopped;

    public MediaCodecVideoDecoder2(String filename, Surface surface) {
        this.filename = filename;
        this.surface = surface;
    }

    public void run(int nbFrames) {
        this.nbFrames = nbFrames;
        try {
            final MediaExtractor extractor = new MediaExtractor();
            extractor.setDataSource(filename);
            extractor.selectTrack(0);

            MediaFormat format = extractor.getTrackFormat(0);
            String mime = format.getString(MediaFormat.KEY_MIME);
            MediaCodec codec = MediaCodec.createDecoderByType(mime);
            codec.configure(format, null, null, 0);
            codec.setCallback(new MediaCodec.Callback() {
                @Override
                public void onInputBufferAvailable(@NonNull MediaCodec codec, int index) {
                    if (stopped)
                        return;
                    ByteBuffer inputBuffer = codec.getInputBuffer(index);
                    int bufferSize = extractor.readSampleData(inputBuffer, 0);
                    if (bufferSize < 0)
                        return;
                    extractor.advance();
                    codec.queueInputBuffer(index, 0, bufferSize, extractor.getSampleTime(), 0);
                }

                @Override
                public void onOutputBufferAvailable(@NonNull MediaCodec codec, int index, @NonNull MediaCodec.BufferInfo info) {
                    codec.releaseOutputBuffer(index, false);
                    frames++;
                    if (frames == 2) {
                        timer = System.currentTimeMillis();
                    }

                }

                @Override
                public void onError(@NonNull MediaCodec codec, @NonNull MediaCodec.CodecException e) {

                }

                @Override
                public void onOutputFormatChanged(@NonNull MediaCodec codec, @NonNull MediaFormat format) {

                }
            });
            codec.start();

            while (this.frames < this.nbFrames) {
                try {
                    Thread.sleep(1);
                } catch (InterruptedException e) {
                    /* pass */
                }
            }

            stopped = true;
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                    /* pass */
            }
            codec.stop();
            codec.release();
        } catch (IOException e) {
            e.printStackTrace();
        }

        Log.e("XXX", "took" + (System.currentTimeMillis() - timer) / 1000.0);
    }
}
