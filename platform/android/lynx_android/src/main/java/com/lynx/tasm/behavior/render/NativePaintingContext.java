// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.PointF;
import android.view.MotionEvent;
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

  @Override
  public void setLynxEngineActorForPlatformRendererContext(long ptr) {
    if (mNativePtr == 0 || mDestroyed) {
      return;
    }
    nativeSetLynxEngineActorForPlatformRendererContext(mNativePtr, ptr);
  }

  @Override
  public boolean dispatchPlatformMotionEvent(MotionEvent ev) {
    if (mNativePtr == 0 || mDestroyed) {
      return false;
    }

    // TODO(hexionghui): handle multi-pointer event.
    // iEventData: [event_type, action_type, event_source, pointer_count, ...]
    int[] iEventData = {0, ev.getActionMasked(), ev.getSource(), ev.getPointerCount()};
    // fEventData: [pointer_id, pointer_x, pointer_y, ...]
    float[] fEventData = {ev.getPointerId(0), ev.getX(), ev.getY()};
    return nativeDispatchPlatformInputEvent(mNativePtr, iEventData, fEventData);
  }

  @Override
  public int getPlatformEventHandlerState() {
    if (mDestroyed || mNativePtr == 0) {
      return 0;
    }
    return nativeGetPlatformEventHandlerState(mNativePtr);
  }

  private native long nativeCreatePaintingContext(
      NativePaintingContext jThis, long platformRendererContextPtr, Object textLayout);

  native void nativeSetLynxEngineActorForPlatformRendererContext(long nativePtr, long ptr);

  native boolean nativeDispatchPlatformInputEvent(
      long nativePtr, int[] iEventData, float[] fEventData);

  native int nativeGetPlatformEventHandlerState(long nativePtr);
}
