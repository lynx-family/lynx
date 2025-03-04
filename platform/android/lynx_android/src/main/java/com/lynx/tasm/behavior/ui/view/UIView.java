// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.view;

import android.content.Context;
import android.graphics.Canvas;
import android.view.View;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.ui.accessibility.CustomAccessibilityDelegateCompat;
import com.lynx.tasm.behavior.ui.utils.MaskDrawable;
import com.lynx.tasm.event.LynxImpressionEvent;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.LynxNewGestureDelegate;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.handler.BaseGestureHandler;
import java.util.HashMap;
import java.util.Map;

public class UIView
    extends UISimpleView<AndroidView> implements GestureArenaMember, LynxNewGestureDelegate {
  // key is gesture id, value is gesture handler
  private Map<Integer, BaseGestureHandler> mGestureHandlers;

  @Deprecated
  public UIView(Context context) {
    super((LynxContext) context);
  }

  public UIView(LynxContext context) {
    super(context);
    if (context.getDefaultOverflowVisible()) {
      mOverflow = OVERFLOW_XY;
    }
  }

  @Override
  public int getInitialOverflowType() {
    return mContext.getDefaultOverflowVisible() ? StyleConstants.OVERFLOW_VISIBLE
                                                : StyleConstants.OVERFLOW_HIDDEN;
  }

  @Override
  protected AndroidView createView(Context context) {
    AndroidView view = onCreateView(context);
    view.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener() {
      @Override
      public void onViewAttachedToWindow(View v) {
        if (v == getView() && mEvents != null && mEvents.containsKey("attach")) {
          LynxImpressionEvent event = LynxImpressionEvent.createAttachEvent(getSign());
          HashMap<String, Object> params = new HashMap<>();
          params.put("impression_id", getView().getImpressionId());
          event.setParmas("params", params);
          if (getLynxContext().getEventEmitter() != null) {
            getLynxContext().getEventEmitter().sendCustomEvent(event);
          }
        }
      }

      @Override
      public void onViewDetachedFromWindow(View v) {
        if (v == getView() && mEvents != null && mEvents.containsKey("detach")) {
          LynxImpressionEvent event = LynxImpressionEvent.createDetachEvent(getSign());
          HashMap<String, Object> params = new HashMap<>();
          params.put("impression_id", getView().getImpressionId());
          event.setParmas("params", params);
          if (getLynxContext().getEventEmitter() != null) {
            getLynxContext().getEventEmitter().sendCustomEvent(event);
          }
        }
      }
    });
    return view;
  }
  @LynxProp(name = "impression_id")
  public void setImpressionId(String value) {
    mView.setImpressionId(value);
  }

  protected AndroidView onCreateView(Context context) {
    return new AndroidView(context);
  }

  @Override
  public boolean enableAutoClipRadius() {
    return true;
  }

  @Override
  public void onPropsUpdated() {
    if (mView != null) {
      mView.setNativeInteractionEnabled(this.nativeInteractionEnabled);
      mView.setConsumeHoverEvent(mConsumeHoverEvent);
    }
    if (mGestureHandlers != null) {
      GestureArenaManager manager = getGestureArenaManager();
      // Check if the current UIList instance is already a member of the gesture arena
      if (manager != null && !manager.isMemberExist(getGestureArenaMemberId())) {
        // If not a member, add the UIList instance as a new member to the gesture arena
        mGestureArenaMemberId = manager.addMember(UIView.this);
      }
    }
    super.onPropsUpdated();
  }

  @Override
  protected void initAccessibilityDelegate() {
    super.initAccessibilityDelegate();
    if (mView != null) {
      ViewCompat.setAccessibilityDelegate(mView, new CustomAccessibilityDelegateCompat(this));
    }
  }

  /**
   * @name: blur-sampling
   * @description: sampling factor when apply blur-filter on bitmap
   * @note: filter:blur
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.8
   **/
  @LynxProp(name = "blur-sampling", defaultInt = 0)
  public void setBlurSampling(int sampling) {
    mView.setBlurSampling(sampling);
  }

  /**
   * @name: copyable
   * @description: Allowing copy LynxUI from one to another at native
   * @category: different
   * @standardAction: keep
   * @supportVersion: 2.11
   **/
  @LynxProp(name = "copyable")
  public void copyable(boolean value) {
    // On Android, LynxUI is copyable by default
  }

  @Override
  public void onGestureScrollBy(float deltaX, float deltaY) {
    // No need to implement if it's not a scrolling container
  }

  @Override
  public boolean canConsumeGesture(float deltaX, float deltaY) {
    // consume gesture by default if it's not a scrolling container
    return true;
  }

  @Override
  public int getMemberScrollX() {
    // always zero if it's not a scrolling container
    return 0;
  }

  @Override
  public boolean isAtBorder(boolean isStart) {
    // always false if it's not a scrolling container
    return false;
  }

  @Override
  public int getMemberScrollY() {
    // always zero if it's not a scrolling container
    return 0;
  }

  @Override
  public void onInvalidate() {
    if (!isEnableNewGesture()) {
      return;
    }
    ViewCompat.postInvalidateOnAnimation(mView);
  }

  @Override
  public void setGestureDetectors(Map<Integer, GestureDetector> gestureDetectors) {
    super.setGestureDetectors(gestureDetectors);
    if (gestureDetectors == null || gestureDetectors.isEmpty()) {
      return;
    }
    GestureArenaManager manager = getGestureArenaManager();
    if (manager == null) {
      return;
    }
    if (manager.isMemberExist(getGestureArenaMemberId())) {
      // when update gesture handlers, need to reset it
      if (mGestureHandlers != null) {
        mGestureHandlers.clear();
        mGestureHandlers = null;
      }
    }
    if (mGestureHandlers == null) {
      mGestureHandlers = BaseGestureHandler.convertToGestureHandler(
          getSign(), getLynxContext(), UIView.this, getGestureDetectorMap());
    }
    mView.setGestureManager(manager);
  }

  @Nullable
  @Override
  public Map<Integer, BaseGestureHandler> getGestureHandlers() {
    if (!isEnableNewGesture()) {
      return null;
    }

    if (mGestureHandlers == null) {
      mGestureHandlers = BaseGestureHandler.convertToGestureHandler(
          getSign(), getLynxContext(), UIView.this, getGestureDetectorMap());
    }
    return mGestureHandlers;
  }
  @Override
  public void destroy() {
    super.destroy();
    // remove arena member if destroy
    GestureArenaManager manager = getGestureArenaManager();
    if (manager != null) {
      manager.removeMember(this);
    }
    // clear gesture map if destroy
    if (mGestureHandlers != null) {
      mGestureHandlers.clear();
    }
  }

  @Override
  public void afterDraw(Canvas canvas) {
    super.afterDraw(canvas);
    if (getMaskDrawable() != null) {
      getMaskDrawable().setBounds(0, 0, getWidth(), getHeight());
      getMaskDrawable().draw(canvas);
    }
  }

  @Nullable
  private MaskDrawable getMaskDrawable() {
    if (mLynxMask == null || mLynxMask.getDrawable() == null) {
      return null;
    }
    return mLynxMask.getDrawable();
  }
}
