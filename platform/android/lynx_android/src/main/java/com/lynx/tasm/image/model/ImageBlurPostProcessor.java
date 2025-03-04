// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image.model;

import android.graphics.Bitmap;
import com.lynx.tasm.utils.BlurUtils;

public class ImageBlurPostProcessor implements BitmapPostProcessor {
  private final int mBlurRadius;
  private String mCacheKey;

  public ImageBlurPostProcessor(int blurRadius) {
    mBlurRadius = blurRadius;
  }

  @Override
  public void process(Bitmap sourceBitmap, Bitmap dstBitmap) {
    BlurUtils.iterativeBoxBlur(dstBitmap, mBlurRadius);
  }

  @Override
  public String getName() {
    return this.getClass().getSimpleName();
  }

  @Override
  public String getPostprocessorCacheKey() {
    if (mCacheKey == null) {
      mCacheKey = String.valueOf(mBlurRadius);
    }
    return mCacheKey;
  }
}
