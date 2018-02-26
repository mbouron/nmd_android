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

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.graphics.SurfaceTexture;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.util.Log;
import android.view.Surface;
import android.view.TextureView;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity implements TextureView.SurfaceTextureListener {

    public static final String TAG = "MainActivity";

    private TextureView[] textureViews;
    private Surface[] surfaces;

    private BroadcastReceiver sceneReceiver = new BroadcastReceiver() {
        @RequiresApi(api = Build.VERSION_CODES.R)
        @Override
        public void onReceive(Context context, Intent intent) {
            if (!Environment.isExternalStorageManager()) {
                Log.e("NGLActivity", "Could not update scene: missing permission");
                Uri uri = Uri.parse("package:" + BuildConfig.APPLICATION_ID);
                startActivity(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, uri));
                return;
             }


            if (surfaces == null) {
                surfaces = new Surface[textureViews.length];
                for (int i = 0; i < textureViews.length; i++) {
                    surfaces[i] = new Surface(textureViews[i].getSurfaceTexture());
                }
            }

            String action = intent.getAction();
            String filename = intent.getStringExtra("filename");
            Log.i(TAG, "Action " + action + " filename= " + filename);

            if (action == "test_audiodecode") {
                testAudioDecode(filename);
            } else if (action == "test_videodecode") {
                int nbDecoders = intent.getIntExtra("nb_decoders", 1);
                int nbFrames = intent.getIntExtra("nb_frames", 600);
                nbDecoders = Math.min(nbDecoders, surfaces.length);
                testVideoDecode(filename, nbDecoders, nbFrames);
            } else if (action == "test_seek") {
                testSeek(filename);
            } else if (action == "test_randomseek") {
                testRandomSeek(filename);
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        int ids[] = {
                R.id.textureView0,
                R.id.textureView1,
                R.id.textureView2,
                R.id.textureView3,
                R.id.textureView4,
                R.id.textureView5,
                R.id.textureView6,
                R.id.textureView7,
        };

        textureViews = new TextureView[ids.length];
        for (int i = 0; i < ids.length; i++) {
            textureViews[i] = findViewById(ids[i]);
            textureViews[i].setSurfaceTextureListener(this);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    == PackageManager.PERMISSION_DENIED) {
                Log.e(TAG, "Could not update scene: missing permission");
                requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0);
            }
        }
    }

    @Override
    protected void onStart() {
        super.onStart();

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction("test_audiodecode");
        intentFilter.addAction("test_videodecode");
        intentFilter.addAction("test_seek");
        intentFilter.addAction("test_randomseek");
        registerReceiver(sceneReceiver, intentFilter);
    }

    @Override
    protected void onStop() {
        super.onStop();

        unregisterReceiver(sceneReceiver);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture, int i, int i1) {
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surfaceTexture, int i, int i1) {
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
        return false;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {
    }

    private void testAudioDecode(final String filename) {
        new Thread() {
            public void run() {
                NopeMD.nativeAudioDecode(filename);
            }
        }.start();
    }

    private void testVideoDecode(final String filename, final int nbDecoders, final int nbFrames) {
        new Thread() {
            public void run() {
                String basename = filename.substring(filename.lastIndexOf("/") + 1);
                final String outputPath = Environment.getExternalStorageDirectory().toString() + "/nmd_data/results/" + Build.MODEL + "-" + basename + "-decode-" + nbDecoders + ".json";
                if (true) {
                    NopeMD.nativeMultipleDecodesToSurfaces(Build.MODEL, filename, surfaces, nbDecoders, 1000, outputPath);
                } else {
                    MediaCodecVideoDecoder ds[] = new MediaCodecVideoDecoder[nbDecoders];
                    for (int i = 0; i < nbDecoders; i++)
                        ds[i] = new MediaCodecVideoDecoder(filename, surfaces[i], i);
                    for (int i = 0; i < nbDecoders; i++) {
                        MediaCodecVideoDecoder d = ds[i];
                        new Thread() {
                            public void run() {
                                d.run(1000);
                            }
                        }.start();
                    }
                }

                //MediaCodecVideoDecoder d = new MediaCodecVideoDecoder(filename, surfaces[0]);
                //d.run(2);
            }
        }.start();
    }

    private void testSeek(final String filename) {
        new Thread() {
            public void run() {
                String basename = filename.substring(filename.lastIndexOf("/") + 1);
                String outputPath = Environment.getExternalStorageDirectory().toString();
                outputPath += "/nmd_data/results/";
                outputPath += Build.MODEL + "-" + basename + "-seek" + ".json";
                NopeMD.nativeSeekAndDecodeToSurfaces(Build.MODEL, filename, surfaces[0], outputPath);
            }
        }.start();
    }

    private void testRandomSeek(final String filename) {
        new Thread() {
            public void run() {
                NopeMD.nativeRandomSeekAndDecodeToSurface(filename, surfaces[0]);
            }
        }.start();
    }
}
