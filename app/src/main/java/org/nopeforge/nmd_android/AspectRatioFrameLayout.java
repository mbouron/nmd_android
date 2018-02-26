
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

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.widget.FrameLayout;


public class AspectRatioFrameLayout extends FrameLayout {

    private double[] ratio = new double[]{1, 1}; // default square

    public AspectRatioFrameLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(attrs);
    }

    public AspectRatioFrameLayout(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(attrs);
    }

    private void init(AttributeSet attrs) {

        TypedArray typedArray = getContext().getTheme()
                .obtainStyledAttributes(attrs, R.styleable.AspectRatioLayout, 0, 0);

        try {
            double aspectRatioW = typedArray.getInt(R.styleable.AspectRatioLayout_aspectRatioW, (int) ratio[0]);
            double aspectRatioH = typedArray.getInt(R.styleable.AspectRatioLayout_aspectRatioH, (int) ratio[1]);

            setAspectRatio(new double[]{aspectRatioW, aspectRatioH});
        } finally {
            typedArray.recycle();
        }
    }

    public void setAspectRatio(double ratio[]) {
        this.ratio = ratio;
        postInvalidate();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {

        if (heightMeasureSpec == 0) { // most likely this view is used in a vertical scrollview
            super.onMeasure(widthMeasureSpec, (int) (widthMeasureSpec / (ratio[0] / ratio[1])));
            return;
        }
        if (widthMeasureSpec == 0) { // most likely this view is used in a horizontal scrollview
            super.onMeasure((int) (heightMeasureSpec * (ratio[0] / ratio[1])), heightMeasureSpec);
            return;
        }

        int originalWidth = MeasureSpec.getSize(widthMeasureSpec);
        int originalHeight = MeasureSpec.getSize(heightMeasureSpec);
        int calculatedHeight = (int) (originalWidth * (1 / (ratio[0] / ratio[1])));

        int finalHeight;
        int finalWidth;

        if (calculatedHeight > originalHeight) {
            finalWidth = (int) (originalHeight * (ratio[0] / ratio[1]));
            finalHeight = originalHeight;
        } else {
            finalWidth = originalWidth;
            finalHeight = calculatedHeight;
        }

        super.onMeasure(
                MeasureSpec.makeMeasureSpec(finalWidth, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(finalHeight, MeasureSpec.EXACTLY));
    }
}