// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.service.image.decoder;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import com.facebook.cache.common.CacheKey;
import com.facebook.cache.common.MultiCacheKey;
import com.facebook.cache.common.SimpleCacheKey;
import com.facebook.common.references.CloseableReference;
import com.facebook.imagepipeline.bitmaps.PlatformBitmapFactory;
import com.facebook.imagepipeline.nativecode.Bitmaps;
import com.facebook.imagepipeline.request.BasePostprocessor;
import com.facebook.imagepipeline.request.Postprocessor;
import com.lynx.tasm.image.model.BitmapPostProcessor;
import java.util.LinkedList;
import java.util.List;

public class MultiPostProcessor implements Postprocessor {
  private final List<BitmapPostProcessor> mPostprocessors;

  private Bitmap.Config mBitmapConfig;

  public MultiPostProcessor(List<BitmapPostProcessor> mPostprocessors, Bitmap.Config config) {
    mBitmapConfig = config;
    this.mPostprocessors = mPostprocessors;
  }

  @Override
  public CloseableReference<Bitmap> process(
      Bitmap sourceBitmap, PlatformBitmapFactory bitmapFactory) {
    CloseableReference<Bitmap> prevBitmap = null, nextBitmap = null;
    try {
      for (BitmapPostProcessor p : mPostprocessors) {
        final Bitmap.Config sourceBitmapConfig =
            mBitmapConfig != null ? mBitmapConfig : sourceBitmap.getConfig();
        nextBitmap =
            bitmapFactory.createBitmapInternal(sourceBitmap.getWidth(), sourceBitmap.getHeight(),
                sourceBitmapConfig != null ? sourceBitmapConfig
                                           : BasePostprocessor.FALLBACK_BITMAP_CONFIGURATION);
        if (nextBitmap.get().getConfig() == sourceBitmap.getConfig()) {
          Bitmaps.copyBitmap(nextBitmap.get(), sourceBitmap);
        } else {
          Canvas canvas = new Canvas(nextBitmap.get());
          canvas.drawBitmap(sourceBitmap, 0, 0, null);
        }
        p.process(prevBitmap != null ? prevBitmap.get() : sourceBitmap, nextBitmap.get());
        CloseableReference.closeSafely(prevBitmap);
        prevBitmap = CloseableReference.cloneOrNull(nextBitmap);
      }
      return CloseableReference.cloneOrNull(nextBitmap);
    } finally {
      CloseableReference.closeSafely(nextBitmap);
    }
  }

  @Override
  public String getName() {
    StringBuilder name = new StringBuilder();
    for (BitmapPostProcessor p : mPostprocessors) {
      if (name.length() > 0) {
        name.append(",");
      }
      name.append(p.getName());
    }
    name.insert(0, "MultiPostProcessor (");
    name.append(")");
    return name.toString();
  }

  @Override
  public CacheKey getPostprocessorCacheKey() {
    LinkedList<CacheKey> keys = new LinkedList<>();
    for (BitmapPostProcessor p : mPostprocessors) {
      keys.push(new SimpleCacheKey(p.getPostprocessorCacheKey()));
    }
    return new MultiCacheKey(keys);
  }
}
