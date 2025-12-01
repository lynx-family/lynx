// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.PointF;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.IPaintingContext;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIBody;

/**
 * Wrap the native object only to manage the lifetime on Java side.
 * All operations are implemented on the native object and called directly
 * by the pipeline.
 */
public class NativePaintingContext implements IPaintingContext {
  private long mNativePtr = 0;

  @NonNull private final PlatformRendererContext mPlatformRendererContext;
  private boolean mDestroyed = false;

  public NativePaintingContext(UIBody.UIBodyView rootView, LynxContext context) {
    mPlatformRendererContext = new PlatformRendererContext(rootView, context);
    mNativePtr = nativeCreatePaintingContext(
        this, mPlatformRendererContext.getNativePtr(), mPlatformRendererContext.getTextLayout());
  }

  @Override
  public void destroy() {
    mDestroyed = true;
    mPlatformRendererContext.destroy();
  }

  @Override
  public long getNativePaintingContextPtr() {
    return mNativePtr;
  }

  @Override
  public PointF convertPointInViewToScreen(int sign, PointF point) {
    return mPlatformRendererContext.convertPointInViewToScreen(sign, point);
  }

  public int getTargetWidth(int sign) {
    return mPlatformRendererContext.getTargetWidth(sign);
  }

  public int getTargetHeight(int sign) {
    return mPlatformRendererContext.getTargetHeight(sign);
  }

  public void attachUIBodyView(UIBody.UIBodyView view) {
    mPlatformRendererContext.setRootView(view);
  }

  private native long nativeCreatePaintingContext(
      NativePaintingContext jThis, long platformRendererContextPtr, Object textLayout);
}
