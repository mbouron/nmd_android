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

import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.util.Log;
import android.view.Surface;

import java.nio.ByteBuffer;

class NopeMD {

    public static final String TAG = "NopeMD";

    public static synchronized void listAvailableCodec() {
        int codecCount = MediaCodecList.getCodecCount();
        for (int i = 0; i < codecCount; i++) {
            MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
            Log.i(TAG, "Codec=" + info.getName());

            String[] supportedTypes = info.getSupportedTypes();
            for (int j = 0; j < supportedTypes.length; j++) {
                MediaCodecInfo.CodecCapabilities caps = info.getCapabilitiesForType(supportedTypes[j]);

                MediaCodecInfo.CodecProfileLevel[] profileLevels = caps.profileLevels;
                Log.i(TAG, "\tProfileLevels.length=" + profileLevels.length);
                for (int k = 0; k < profileLevels.length; k++) {
                    MediaCodecInfo.CodecProfileLevel profileLevel = profileLevels[k];
                }
            }
        }
    }

    public static void dumpExtradata(String filename) {
        try {
            MediaExtractor extractor = new MediaExtractor();
            extractor.setDataSource(filename);
            MediaFormat format = extractor.getTrackFormat(0);
            ByteBuffer csd0 = format.getByteBuffer("csd-0");
            ByteBuffer csd1 = format.getByteBuffer("csd-1");
            Log.i(TAG, format.toString());
            Log.i(TAG, format.getString(MediaFormat.KEY_MIME) + ", ");
            Log.i(TAG, format.getInteger(MediaFormat.KEY_WIDTH) + ", ");
            Log.i(TAG, format.getInteger(MediaFormat.KEY_HEIGHT) + ", ");
            Log.i(TAG, format.getInteger(MediaFormat.KEY_PROFILE) + ", ");
            Log.i(TAG, format.getInteger(MediaFormat.KEY_LEVEL) + ", ");
            Log.i(TAG, "byte[] {" + convertExtradataToString(csd0) + "}, ");
            Log.i(TAG, "byte[] {" + convertExtradataToString(csd1) + "}");
            Log.i(TAG, ">>");
            extractor.release();
        } catch (Exception e) {
            Log.e(TAG, "Error while dumping extradata", e);
        }
    }

    private static String convertExtradataToString(ByteBuffer buffer) {
        byte[] data = new byte[buffer.limit()];
        buffer.get(data);
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < data.length; i++) {
            byte b = data[i];
            sb.append(String.format("%d, ", b));
        }
        return sb.toString();
    }

    public native static void nativeDecodeAllFramesToSurface(String filename,
                                                             int live,
                                                             Surface surface);

    public native static synchronized void nativeMultipleDecodesToSurfaces(String model,
                                                                           String filename,
                                                                           Surface[] surfaces,
                                                                           int nbSurfaces,
                                                                           int nbFrames,
                                                                           String outputPath);

    public native static synchronized void nativeSeekAndDecodeToSurfaces(String model,
                                                                         String filename,
                                                                         Surface surface,
                                                                         String outputPath);

    public native static synchronized void nativeRandomSeekAndDecodeToSurface(String filename,
                                                                              Surface surface);

    public native static synchronized void nativeAudioDecode(String filename);
}
