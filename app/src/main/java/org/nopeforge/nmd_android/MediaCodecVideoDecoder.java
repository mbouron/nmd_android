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

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Random;

public class MediaCodecVideoDecoder extends Thread {
    private final String filename;
    private final Surface surface;
    private final long index;

    public MediaCodecVideoDecoder(String filename, Surface surface, long index) {
        this.filename = filename;
        this.surface = surface;
        this.index = index;
    }

    public void run(int nbFrames) {
        long timer = System.currentTimeMillis();
        try {
            MediaExtractor extractor = new MediaExtractor();
            extractor.setDataSource(filename);
            extractor.selectTrack(0);

            MediaFormat format = extractor.getTrackFormat(0);
            String mime = format.getString(MediaFormat.KEY_MIME);
            MediaCodec codec = MediaCodec.createDecoderByType(mime);
            codec.configure(format, surface, null, 0);
            codec.start();
            Random random2 = new Random();
            int randomNumber2 = random2.nextInt(120-80) + 80;
            //extractor.seekTo(0 * 1000000, MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
            int i = 0;
            boolean gotOutput = false;
            MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
            while (i < nbFrames) {
                if (i % randomNumber2 == 0) {
                    codec.flush();
                    Random random = new Random();
                    int randomNumber = random.nextInt(120-1) + 1;
                    extractor.seekTo(randomNumber * 1000000, MediaExtractor.SEEK_TO_PREVIOUS_SYNC);
                    i++;
                }

                int index = codec.dequeueInputBuffer(8000);
                if (index == MediaCodec.INFO_TRY_AGAIN_LATER) {
                    /* pass */
                } else if (index >= 0) {
                    ByteBuffer inputBuffer = codec.getInputBuffer(index);
                    int bufferSize = extractor.readSampleData(inputBuffer, 0);
                    extractor.advance();
                    if (bufferSize < 0) {
                        break;
                    } else {
                        codec.queueInputBuffer(index, 0, bufferSize, extractor.getSampleTime(), 0);
                    }
                } else {
                    Log.e("XXX", "Mediacodec returned an error while getting an input buffer: " + index);
                }

                index = codec.dequeueOutputBuffer(info, gotOutput ? 12000 : 4000);
                if (index == MediaCodec.INFO_TRY_AGAIN_LATER) {
                } else if (index >= 0) {
                    if (gotOutput) {
                        codec.releaseOutputBuffer(index, true);
                        if (i == 0) {
                            timer = System.currentTimeMillis();
                        }
                        i++;

                    }
                    gotOutput = true;
                } else if (index != MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED && index != MediaCodec.INFO_OUTPUT_FORMAT_CHANGED){
                    Log.e("XXX", "MediaCodec returned while getting an output buffer: " + index);
                }
            }
            codec.flush();
            codec.stop();
            codec.release();
        } catch (IOException e) {
            e.printStackTrace();
        }

        Log.e("XXX", "took" + (double)(System.currentTimeMillis() - timer) / 1000.0);
    }
}
