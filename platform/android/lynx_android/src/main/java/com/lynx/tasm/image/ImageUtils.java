// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image;

import android.graphics.Bitmap;
import android.text.TextUtils;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableType;

public class ImageUtils {
  public static final int ALPHA_8_BYTES_ONE_PIXEL = 1;
  public static final int ARGB_4444_BYTES_ONE_PIXEL = 2;
  public static final int ARGB_8888_BYTES_ONE_PIXEL = 4;
  public static final int RGB_565_BYTES_ONE_PIXEL = 2;

  public static class LocalCacheState {
    public boolean mUseLocalCache = false;
    public boolean mAwaitLocalCache = false;
  }
  public static LocalCacheState parseLocalCache(Dynamic localCache) {
    LocalCacheState state = new LocalCacheState();

    if (localCache == null) {
      state.mUseLocalCache = false;
    } else {
      ReadableType type = localCache.getType();
      if (type == ReadableType.Boolean) {
        // when only using boolean, just set the value to mUseLocalCache
        // The meaning of true is to use the image resources provided by the container
        state.mUseLocalCache = localCache.asBoolean();
        state.mAwaitLocalCache = false;
      } else if (type == ReadableType.String) {
        // when using string,there will be the following situations
        // 1."true"/"false": same as setting boolean,set the value to mUseLocalCache
        // 2."default": The meaning is the same as "true"/true
        // 3."await": The meaning is to wait for the asynchronous callback of the container's image
        // resources to complete
        // 4."none": The meaning is the same as "false"/false
        String cacheState = localCache.asString();
        if (TextUtils.isEmpty(cacheState) || "none".equals(cacheState)
            || "false".equals(cacheState)) {
          state.mUseLocalCache = false;
          state.mAwaitLocalCache = false;
        } else if ("default".equals(cacheState) || "true".equals(cacheState)) {
          state.mUseLocalCache = true;
          state.mAwaitLocalCache = false;
        } else if ("await".equals(cacheState)) {
          state.mUseLocalCache = true;
          state.mAwaitLocalCache = true;
        }
      }
    }
    return state;
  }

  private static int getPixelSizeForBitmapConfig(Bitmap.Config bitmapConfig) {
    switch (bitmapConfig) {
      case ARGB_8888:
        return ARGB_8888_BYTES_ONE_PIXEL;
      case ALPHA_8:
        return ALPHA_8_BYTES_ONE_PIXEL;
      case ARGB_4444:
        return ARGB_4444_BYTES_ONE_PIXEL;
      case RGB_565:
        return RGB_565_BYTES_ONE_PIXEL;
    }
    throw new UnsupportedOperationException("Current Bitmap.Config is not supported");
  }

  public static int getSizeInByteForBitmap(int width, int height, Bitmap.Config bitmapConfig) {
    return width * height * getPixelSizeForBitmapConfig(bitmapConfig);
  }
}
