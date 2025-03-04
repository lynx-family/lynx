// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image.model;

import android.graphics.Bitmap;

public interface BitmapPostProcessor {
  void process(Bitmap sourceBitmap, Bitmap dstBitmap);

  String getName();

  String getPostprocessorCacheKey();
}
