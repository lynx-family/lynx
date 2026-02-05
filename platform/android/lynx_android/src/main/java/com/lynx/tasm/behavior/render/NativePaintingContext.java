// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.PointF;
import android.view.MotionEvent;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.BehaviorRegistry;
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
  private long mTextra = 0;
  private LynxContext mContext;

  public NativePaintingContext(
      UIBody.UIBodyView rootView, LynxContext context, BehaviorRegistry behaviorRegistry) {
    mPlatformRendererContext = new PlatformRendererContext(rootView, context, behaviorRegistry);
    mContext = context;
    if (context.isTextServiceModeOn() && context.getTextService() != null) {
      mTextra = context.getTextService().createTextLayoutAPI(context);
    }
    mNativePtr = nativeCreatePaintingContext(this, mPlatformRendererContext.getNativePtr(),
        mPlatformRendererContext.getTextLayout(), mTextra);
  }

  // this func will be execed on main thread.
  @Override
  public void destroy() {
    mDestroyed = true;
    mPlatformRendererContext.destroy();
    if (mTextra != 0 && mContext != null && mContext.getTextService() != null) {
      mContext.getTextService().destroyTextLayoutAPI(mTextra);
    }
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
  public void setLynxEngineActorForPlatformContextRef(long ptr) {
    if (mNativePtr == 0 || mDestroyed) {
      return;
    }
    nativeSetLynxEngineActorForPlatformContextRef(mNativePtr, ptr);
  }

  @Override
  public boolean dispatchPlatformMotionEvent(MotionEvent ev) {
    if (mNativePtr == 0 || mDestroyed) {
      return false;
    }

    int pointerCount = ev.getPointerCount();
    // iEventData: [event_type, action_type, event_source, pointer_count, ...]
    int[] iEventData = {0, ev.getActionMasked(), ev.getSource(), pointerCount};
    // fEventData: [pointer_id, pointer_x, pointer_y, ...]
    float[] fEventData = new float[pointerCount * 3];
    for (int i = 0; i < pointerCount; i++) {
      int base = i * 3;
      fEventData[base] = ev.getPointerId(i);
      fEventData[base + 1] = ev.getX(i);
      fEventData[base + 2] = ev.getY(i);
    }
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
      NativePaintingContext jThis, long platformRendererContextPtr, Object textLayout, long textra);

  native void nativeSetLynxEngineActorForPlatformContextRef(long nativePtr, long ptr);

  native boolean nativeDispatchPlatformInputEvent(
      long nativePtr, int[] iEventData, float[] fEventData);

  native int nativeGetPlatformEventHandlerState(long nativePtr);
}
