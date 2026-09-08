// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.PointF;
import android.view.MotionEvent;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.IPaintingContext;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.MeaningfulPaintingArea;
import com.lynx.tasm.behavior.ui.UIBody;
import java.util.ArrayList;
import java.util.List;

/**
 * Wrap the native object only to manage the lifetime on Java side.
 * All operations are implemented on the native object and called directly
 * by the pipeline.
 */
public class NativePaintingContext extends PlatformRendererContext implements IPaintingContext {
  private static final int PLATFORM_FOCUS_INFO_SIZE = 4;
  private static final int PLATFORM_FOCUS_TARGET_SIGN_INDEX = 0;
  private static final int PLATFORM_FOCUS_RENDERER_HOST_SIGN_INDEX = 1;
  private static final int PLATFORM_FOCUS_IGNORE_INDEX = 2;
  private static final int PLATFORM_FOCUS_CAN_RESPOND_INDEX = 3;

  private long mNativePtr = 0;

  private boolean mDestroyed = false;
  private long mTextra = 0;

  public NativePaintingContext(
      UIBody.UIBodyView rootView, LynxContext context, BehaviorRegistry behaviorRegistry) {
    super(rootView, context, behaviorRegistry);
    if (context.isTextServiceModeOn() && context.getTextService() != null) {
      mTextra = context.getTextService().createTextLayoutAPI(context);
    }
    mNativePtr = nativeCreatePaintingContext(this, getNativePtr(), getTextLayout(), mTextra);
  }

  @Override
  public void destroy() {
    if (mDestroyed) {
      return;
    }
    mDestroyed = true;

    if (mNativePtr != 0) {
      nativeDestroy(mNativePtr);
      mNativePtr = 0;
    }
    super.destroy();
    // TextLayoutTextra owns mTextra and releases it on native teardown.
    mTextra = 0;
  }

  @Override
  public long getNativePaintingContextPtr() {
    return mNativePtr;
  }

  @Override
  public PointF convertPointInViewToScreen(int sign, PointF point) {
    return super.convertPointInViewToScreen(sign, point);
  }

  @Override
  public int getTargetWidth(int sign) {
    return super.getTargetWidth(sign);
  }

  @Override
  public int getTargetHeight(int sign) {
    return super.getTargetHeight(sign);
  }

  public void attachUIBodyView(UIBody.UIBodyView view) {
    setRootView(view);
  }

  @Override
  public void setLynxEngineActorForPlatformContextRef(long ptr) {
    if (mNativePtr == 0 || mDestroyed) {
      return;
    }
    nativeSetLynxEngineActorForPlatformContextRef(mNativePtr, ptr);
  }

  @Override
  public boolean dispatchPlatformMotionEvent(MotionEvent ev, int rootSign) {
    if (mNativePtr == 0 || mDestroyed) {
      return false;
    }

    int actionMasked = ev.getActionMasked();
    int actionType = getPlatformActionType(actionMasked);
    // Pointer down/up MotionEvents contain all active pointers, while native down/up
    // handlers mutate state for every pointer in the payload.
    boolean dispatchActionPointerOnly = isActionPointerEvent(actionMasked);
    int pointerCount = dispatchActionPointerOnly ? 1 : ev.getPointerCount();
    // iEventData: [event_type, action_type, event_source, pointer_count, root_sign, ...]
    int[] iEventData = {0, actionType, ev.getSource(), pointerCount, rootSign};
    // fEventData: [pointer_id, pointer_x, pointer_y, ...]
    float[] fEventData = new float[pointerCount * 3];
    for (int i = 0; i < pointerCount; i++) {
      int pointerIndex = dispatchActionPointerOnly ? ev.getActionIndex() : i;
      int base = i * 3;
      fEventData[base] = ev.getPointerId(pointerIndex);
      fEventData[base + 1] = ev.getX(pointerIndex);
      fEventData[base + 2] = ev.getY(pointerIndex);
    }
    return nativeDispatchPlatformInputEvent(mNativePtr, iEventData, fEventData);
  }

  @Override
  public void dispatchPlatformLongPress() {
    if (mNativePtr == 0 || mDestroyed) {
      return;
    }
    nativeDispatchPlatformLongPress(mNativePtr);
  }

  @Override
  public void dispatchPlatformTap() {
    if (mNativePtr == 0 || mDestroyed) {
      return;
    }
    nativeDispatchPlatformTap(mNativePtr);
  }

  @Override
  public void dispatchPlatformFocus() {
    if (mNativePtr == 0 || mDestroyed) {
      return;
    }
    handlePlatformFocusInfo(nativeGetPlatformFocusInfo(mNativePtr));
  }

  void handlePlatformFocusInfo(@Nullable int[] focusInfo) {
    if (focusInfo == null || focusInfo.length < PLATFORM_FOCUS_INFO_SIZE
        || focusInfo[PLATFORM_FOCUS_IGNORE_INDEX] != 0
        || focusInfo[PLATFORM_FOCUS_CAN_RESPOND_INDEX] == 0) {
      return;
    }
    updatePlatformFocus(focusInfo[PLATFORM_FOCUS_TARGET_SIGN_INDEX],
        focusInfo[PLATFORM_FOCUS_RENDERER_HOST_SIGN_INDEX]);
  }

  @Override
  public boolean isPlatformEventTargetEventThrough(int rootSign, float pointX, float pointY) {
    if (mNativePtr == 0 || mDestroyed) {
      return false;
    }
    return nativeIsPlatformEventTargetEventThrough(mNativePtr, rootSign, pointX, pointY);
  }

  private static int getPlatformActionType(int actionMasked) {
    if (actionMasked == MotionEvent.ACTION_POINTER_DOWN) {
      return MotionEvent.ACTION_DOWN;
    }
    if (actionMasked == MotionEvent.ACTION_POINTER_UP) {
      return MotionEvent.ACTION_UP;
    }
    return actionMasked;
  }

  private static boolean isActionPointerEvent(int actionMasked) {
    return actionMasked == MotionEvent.ACTION_POINTER_DOWN
        || actionMasked == MotionEvent.ACTION_POINTER_UP;
  }

  public void setPlatformEventRootActive(int rootSign, boolean active) {
    if (mNativePtr == 0 || mDestroyed) {
      return;
    }
    nativeSetPlatformEventRootActive(mNativePtr, rootSign, active);
  }

  public void setPlatformEventRootOffset(int rootSign, float offsetX, float offsetY) {
    if (mNativePtr == 0 || mDestroyed) {
      return;
    }
    nativeSetPlatformEventRootOffset(mNativePtr, rootSign, offsetX, offsetY);
  }

  public List<MeaningfulPaintingArea> getMeaningfulPaintingAreas() {
    if (mDestroyed || mNativePtr == 0) {
      return new ArrayList<>();
    }

    return MeaningfulPaintingAreaHelper.buildMeaningfulPaintingAreas(
        nativeGetMeaningfulPaintingAreaRecords(mNativePtr), this, getLynxContext());
  }

  private native long nativeCreatePaintingContext(
      NativePaintingContext jThis, long platformRendererContextPtr, Object textLayout, long textra);

  native void nativeSetLynxEngineActorForPlatformContextRef(long nativePtr, long ptr);

  native boolean nativeDispatchPlatformInputEvent(
      long nativePtr, int[] iEventData, float[] fEventData);

  native void nativeDispatchPlatformLongPress(long nativePtr);

  native void nativeDispatchPlatformTap(long nativePtr);

  native int[] nativeGetPlatformFocusInfo(long nativePtr);

  native void nativeSetPlatformEventRootActive(long nativePtr, int rootSign, boolean active);

  native void nativeSetPlatformEventRootOffset(
      long nativePtr, int rootSign, float offsetX, float offsetY);

  native boolean nativeIsPlatformEventTargetEventThrough(
      long nativePtr, int rootSign, float pointX, float pointY);

  private native void nativeDestroy(long nativePtr);

  native int[] nativeGetMeaningfulPaintingAreaRecords(long nativePtr);
}
