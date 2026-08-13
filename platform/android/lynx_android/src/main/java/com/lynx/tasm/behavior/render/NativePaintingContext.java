// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.PointF;
import android.os.Build;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import androidx.annotation.NonNull;
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
  private static final int INVALID_POINTER_ID = -1;

  private long mNativePtr = 0;

  @NonNull private final PlatformRendererContext mPlatformRendererContext;
  private boolean mDestroyed = false;
  private long mTextra = 0;
  private LynxContext mContext;
  private int mPrimaryPointerId = INVALID_POINTER_ID;

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
    if (mNativePtr == 0 || mDestroyed) {
      return false;
    }

    int actionMasked = ev.getActionMasked();
    if (actionMasked == MotionEvent.ACTION_SCROLL) {
      return dispatchPlatformWheelEvent(ev, rootSign);
    }
    int actionType = getPlatformActionType(actionMasked);
    if (actionMasked == MotionEvent.ACTION_DOWN) {
      mPrimaryPointerId = ev.getPointerId(ev.getActionIndex());
    }
    // Pointer down/up MotionEvents contain all active pointers, while native down/up
    // handlers mutate state for every pointer in the payload.
    boolean dispatchActionPointerOnly = isActionPointerEvent(actionMasked);
    int pointerCount = dispatchActionPointerOnly ? 1 : ev.getPointerCount();
    // iEventData: [event_type, action_type, event_source, pointer_count, root_sign, ...]
    int[] iEventData = {0, actionType, ev.getSource(), pointerCount, rootSign};
    // fEventData: [pointer_id, pointer_x, pointer_y, pointer_type, is_primary, button, buttons, ...]
    float[] fEventData = new float[pointerCount * 7];
    for (int i = 0; i < pointerCount; i++) {
      int pointerIndex = dispatchActionPointerOnly ? ev.getActionIndex() : i;
      int pointerId = ev.getPointerId(pointerIndex);
      int base = i * 7;
      fEventData[base] = ev.getPointerId(pointerIndex);
      fEventData[base + 1] = ev.getX(pointerIndex);
      fEventData[base + 2] = ev.getY(pointerIndex);
      fEventData[base + 3] = getPointerType(ev, pointerIndex);
      fEventData[base + 4] = isPrimaryPointer(pointerId, actionMasked) ? 1 : 0;
      fEventData[base + 5] = getWebButton(ev);
      fEventData[base + 6] = getWebButtons(ev, pointerIndex);
    }
    boolean handled = nativeDispatchPlatformInputEvent(mNativePtr, iEventData, fEventData);
    if (actionMasked == MotionEvent.ACTION_UP || actionMasked == MotionEvent.ACTION_CANCEL
        || (actionMasked == MotionEvent.ACTION_POINTER_UP
            && ev.getPointerId(ev.getActionIndex()) == mPrimaryPointerId)) {
      mPrimaryPointerId = INVALID_POINTER_ID;
    }
    return handled;
  }

  @Override
  public boolean dispatchPlatformKeyEvent(KeyEvent event, String key, int rootSign) {
    if (mNativePtr == 0 || mDestroyed) {
      return false;
    }
    int action;
    if (event.isCanceled()) {
      action = 2;
    } else if (event.getAction() == KeyEvent.ACTION_DOWN) {
      action = 0;
    } else if (event.getAction() == KeyEvent.ACTION_UP) {
      action = 1;
    } else {
      return false;
    }
    int keyLength = key.length();
    int[] iEventData = new int[6 + keyLength];
    iEventData[0] = 1;
    iEventData[1] = action;
    iEventData[2] = event.getKeyCode();
    iEventData[3] = event.getRepeatCount() > 0 ? 1 : 0;
    iEventData[4] = rootSign;
    iEventData[5] = keyLength;
    for (int i = 0; i < keyLength; i++) {
      iEventData[6 + i] = key.charAt(i);
    }
    float[] fEventData = {event.isAltPressed() ? 1 : 0, event.isCtrlPressed() ? 1 : 0,
        event.isShiftPressed() ? 1 : 0, event.isMetaPressed() ? 1 : 0};
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
    if (actionMasked == MotionEvent.ACTION_HOVER_MOVE) {
      return MotionEvent.ACTION_MOVE;
    }
    return actionMasked;
  }

  private static boolean isActionPointerEvent(int actionMasked) {
    return actionMasked == MotionEvent.ACTION_POINTER_DOWN
        || actionMasked == MotionEvent.ACTION_POINTER_UP;
  }

  private boolean isPrimaryPointer(int pointerId, int actionMasked) {
    if (mPrimaryPointerId != INVALID_POINTER_ID) {
      return pointerId == mPrimaryPointerId;
    }
    return actionMasked == MotionEvent.ACTION_HOVER_MOVE || actionMasked == MotionEvent.ACTION_SCROLL;
  }

  private static int getPointerType(MotionEvent ev, int pointerIndex) {
    switch (ev.getToolType(pointerIndex)) {
      case MotionEvent.TOOL_TYPE_MOUSE:
        return 1;
      case MotionEvent.TOOL_TYPE_STYLUS:
      case MotionEvent.TOOL_TYPE_ERASER:
        return 2;
      case MotionEvent.TOOL_TYPE_FINGER:
      case MotionEvent.TOOL_TYPE_UNKNOWN:
      default:
        return 0;
    }
  }

  private static int getWebButton(MotionEvent ev) {
    int action = ev.getActionMasked();
    if (action != MotionEvent.ACTION_DOWN && action != MotionEvent.ACTION_POINTER_DOWN
        && action != MotionEvent.ACTION_UP && action != MotionEvent.ACTION_POINTER_UP) {
      return -1;
    }
    int actionButton = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? ev.getActionButton() : 0;
    if (actionButton == MotionEvent.BUTTON_TERTIARY) {
      return 1;
    }
    if (actionButton == MotionEvent.BUTTON_SECONDARY) {
      return 2;
    }
    if (actionButton == MotionEvent.BUTTON_BACK) {
      return 3;
    }
    if (actionButton == MotionEvent.BUTTON_FORWARD) {
      return 4;
    }
    return 0;
  }

  private static int getWebButtons(MotionEvent ev, int pointerIndex) {
    int action = ev.getActionMasked();
    if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP
        || action == MotionEvent.ACTION_CANCEL || action == MotionEvent.ACTION_HOVER_MOVE
        || action == MotionEvent.ACTION_HOVER_ENTER || action == MotionEvent.ACTION_HOVER_EXIT) {
      return 0;
    }
    if (ev.getToolType(pointerIndex) != MotionEvent.TOOL_TYPE_MOUSE) {
      return 1;
    }
    int androidButtons = ev.getButtonState();
    int webButtons = 0;
    if ((androidButtons & MotionEvent.BUTTON_PRIMARY) != 0) {
      webButtons |= 1;
    }
    if ((androidButtons & MotionEvent.BUTTON_SECONDARY) != 0) {
      webButtons |= 2;
    }
    if ((androidButtons & MotionEvent.BUTTON_TERTIARY) != 0) {
      webButtons |= 4;
    }
    if ((androidButtons & MotionEvent.BUTTON_BACK) != 0) {
      webButtons |= 8;
    }
    if ((androidButtons & MotionEvent.BUTTON_FORWARD) != 0) {
      webButtons |= 16;
    }
    return webButtons;
  }

  private boolean dispatchPlatformWheelEvent(MotionEvent ev, int rootSign) {
    int[] iEventData = {2, 0, ev.getSource(), 1, rootSign};
    float[] fEventData = {ev.getX(), ev.getY(),
        -ev.getAxisValue(MotionEvent.AXIS_HSCROLL) * getScrollFactor(true),
        -ev.getAxisValue(MotionEvent.AXIS_VSCROLL) * getScrollFactor(false)};
    nativeDispatchPlatformInputEvent(mNativePtr, iEventData, fEventData);
    iEventData[1] = 1;
    boolean handled = nativeDispatchPlatformInputEvent(mNativePtr, iEventData, fEventData);
    iEventData[1] = 2;
    nativeDispatchPlatformInputEvent(mNativePtr, iEventData, fEventData);
    return handled;
  }

  private float getScrollFactor(boolean horizontal) {
    ViewConfiguration configuration = ViewConfiguration.get(mContext);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      if (horizontal) {
        return configuration.getScaledHorizontalScrollFactor();
      }
      return configuration.getScaledVerticalScrollFactor();
    }
    TypedValue value = new TypedValue();
    if (mContext.getTheme().resolveAttribute(
            android.R.attr.listPreferredItemHeight, value, true)) {
      return value.getDimension(mContext.getResources().getDisplayMetrics());
    }
    return 1;
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

  @Override
  public int getPlatformEventHandlerState() {
    if (mDestroyed || mNativePtr == 0) {
      return 0;
    }
    return nativeGetPlatformEventHandlerState(mNativePtr);
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

  native boolean nativeDispatchPlatformInputEvent(
      long nativePtr, int[] iEventData, float[] fEventData);

  native void nativeDispatchPlatformLongPress(long nativePtr);

  native void nativeDispatchPlatformTap(long nativePtr);

  native void nativeSetPlatformEventRootActive(long nativePtr, int rootSign, boolean active);

  native void nativeSetPlatformEventRootOffset(
      long nativePtr, int rootSign, float offsetX, float offsetY);

  native boolean nativeIsPlatformEventTargetEventThrough(
      long nativePtr, int rootSign, float pointX, float pointY);

  native int nativeGetPlatformEventHandlerState(long nativePtr);

  native void nativeDestroy(long nativePtr);

  native int[] nativeGetMeaningfulPaintingAreaRecords(long nativePtr);
}
