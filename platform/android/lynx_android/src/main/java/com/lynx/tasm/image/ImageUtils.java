// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image;

import android.graphics.Bitmap;
import android.graphics.Rect;
import android.text.TextUtils;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.base.LLog;

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

  public static @Nullable Rect parseRegionToDecode(@Nullable ReadableMap region) {
    if (region == null) {
      return null;
    }

    Rect rect = null;
    try {
      if (region.hasKey("x") && region.hasKey("y") && region.hasKey("width")
          && region.hasKey("height")) {
        int left = (int) region.getDouble("x");
        int top = (int) region.getDouble("y");
        int width = (int) region.getDouble("width");
        int height = (int) region.getDouble("height");
        rect = new Rect(left, top, left + width, top + height);
      }
    } catch (Throwable e) {
      LLog.e("ImageUtils", "parseRegionToDecode error: " + e.getMessage());
    }

    if (rect == null) {
      LLog.w("ImageUtils", "parseRegionToDecode: rect is null");
      return null;
    }

    if (rect.left < 0 || rect.top < 0 || rect.right <= rect.left || rect.bottom <= rect.top) {
      LLog.w("ImageUtils", "parseRegionToDecode: invalid rect " + rect);
      return null;
    }

    return rect;
  }
}
