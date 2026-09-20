// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.PointF;
import android.view.MotionEvent;
import androidx.annotation.NonNull;
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
public class NativePaintingContext implements IPaintingContext {
  private static final int PLATFORM_EVENT_TARGET_INFO_SIZE = 2;
  private static final int PLATFORM_EVENT_TARGET_SIGN_INDEX = 0;
  private static final int PLATFORM_EVENT_RENDERER_HOST_SIGN_INDEX = 1;

  private long mNativePtr = 0;
  private int mEventBehavior = EVENT_BEHAVIOR_NONE;
  @Nullable private int[] mPlatformEventTargetInfo;

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

  @Override
  public void destroy() {
    if (mDestroyed) {
      return;
    }
    mDestroyed = true;
    mEventBehavior = EVENT_BEHAVIOR_NONE;
    mPlatformEventTargetInfo = null;

    if (mNativePtr != 0) {
      nativeDestroy(mNativePtr);
      mNativePtr = 0;
    }
    mPlatformRendererContext.destroy();
    // TextLayoutTextra owns mTextra and releases it on native teardown.
    mTextra = 0;
    mContext = null;
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
  public boolean dispatchPlatformMotionEvent(MotionEvent ev, int rootSign) {
    if (ev.getActionMasked() == MotionEvent.ACTION_DOWN) {
      mEventBehavior = EVENT_BEHAVIOR_NONE;
      mPlatformEventTargetInfo = null;
    }
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
    int eventBehavior = nativeDispatchPlatformInputEvent(mNativePtr, iEventData, fEventData);
    if (actionMasked == MotionEvent.ACTION_DOWN) {
      mEventBehavior = eventBehavior;
    }
    boolean consumed = (mEventBehavior & EVENT_BEHAVIOR_EVENT_THROUGH) == 0;
    if (actionMasked == MotionEvent.ACTION_DOWN) {
      if (consumed) {
        mPlatformEventTargetInfo = nativeGetPlatformEventTargetInfo(mNativePtr);
      }
    }
    return consumed;
  }

  @Override
  public int getPlatformEventBehavior() {
    return mEventBehavior;
  }

  @Override
  public int hitTestAndCachePlatformEventBehavior(int rootSign, float pointX, float pointY) {
    mEventBehavior = EVENT_BEHAVIOR_NONE;
    mPlatformEventTargetInfo = null;
    if (mNativePtr == 0 || mDestroyed) {
      return mEventBehavior;
    }
    mEventBehavior =
        nativeHitTestAndCachePlatformEventBehavior(mNativePtr, rootSign, pointX, pointY);
    return mEventBehavior;
  }

  @Override
  public int getPlatformTouchTargetSign() {
    if (mNativePtr == 0 || mDestroyed) {
      return -1;
    }
    return mPlatformEventTargetInfo != null
            && mPlatformEventTargetInfo.length >= PLATFORM_EVENT_TARGET_INFO_SIZE
        ? mPlatformEventTargetInfo[PLATFORM_EVENT_TARGET_SIGN_INDEX]
        : -1;
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
    if (mNativePtr == 0 || mDestroyed || mPlatformEventTargetInfo == null
        || mPlatformEventTargetInfo.length < PLATFORM_EVENT_TARGET_INFO_SIZE
        || (mEventBehavior & EVENT_BEHAVIOR_IGNORE_FOCUS) != 0
        || !nativeCanRespondPlatformFocus(mNativePtr)) {
      return;
    }
    mPlatformRendererContext.updatePlatformFocus(
        mPlatformEventTargetInfo[PLATFORM_EVENT_TARGET_SIGN_INDEX],
        mPlatformEventTargetInfo[PLATFORM_EVENT_RENDERER_HOST_SIGN_INDEX]);
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
        nativeGetMeaningfulPaintingAreaRecords(mNativePtr), mPlatformRendererContext, mContext);
  }

  private native long nativeCreatePaintingContext(
      NativePaintingContext jThis, long platformRendererContextPtr, Object textLayout, long textra);

  native void nativeSetLynxEngineActorForPlatformContextRef(long nativePtr, long ptr);

  native int nativeDispatchPlatformInputEvent(long nativePtr, int[] iEventData, float[] fEventData);

  native void nativeDispatchPlatformLongPress(long nativePtr);

  native void nativeDispatchPlatformTap(long nativePtr);

  native int[] nativeGetPlatformEventTargetInfo(long nativePtr);

  native int nativeHitTestAndCachePlatformEventBehavior(
      long nativePtr, int rootSign, float pointX, float pointY);

  native boolean nativeCanRespondPlatformFocus(long nativePtr);

  native void nativeSetPlatformEventRootActive(long nativePtr, int rootSign, boolean active);

  native void nativeSetPlatformEventRootOffset(
      long nativePtr, int rootSign, float offsetX, float offsetY);

  native void nativeDestroy(long nativePtr);

  native int[] nativeGetMeaningfulPaintingAreaRecords(long nativePtr);
}
