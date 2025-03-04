// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Matrix;
import android.text.TextPaint;
import android.text.style.ClickableSpan;
import android.view.MotionEvent;
import android.view.View;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.event.EventTargetBase;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxEventDetail;
import com.lynx.tasm.gesture.detector.GestureDetector;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

public class EventTargetSpan extends ClickableSpan implements EventTarget {
  private int mSign;

  private int mPseudoStatus;

  // Since TextRendererCache is a LRU cache, it may hold this EventTargetSpan.
  // TextRendererCache -> EventTargetSpan -> FlattenUIText -> LynxContext, and
  // LynxContext may be Activity. In this case, the Activity will leak. To fix
  // this leak, let EventTargetSpan hold WeakReference parent.
  // Detail can be seen issue: #5543
  private WeakReference<EventTarget> mParent;
  private Map<String, EventsListener> mEvents;
  private EventTarget.EnableStatus mIgnoreFocus;
  private EventTarget.EnableStatus mEventThrough;
  private ReadableMap mDataset = new JavaOnlyMap();
  private Matrix mTransformMatrix = new Matrix();
  private boolean mEnableTouchPseudoPropagation;

  public EventTargetSpan(int sign, Map<String, EventsListener> events,
      EventTarget.EnableStatus ignoreFocus, boolean enableTouchPseudoPropagation,
      EventTarget.EnableStatus eventThrough, ReadableMap dataset) {
    mSign = sign;
    mParent = null;
    mIgnoreFocus = ignoreFocus;
    mEventThrough = eventThrough;
    mDataset = dataset;
    mEnableTouchPseudoPropagation = enableTouchPseudoPropagation;
    if (events != null) {
      // Copy events to mEvents to avoid synchronization problem.
      // This is a shallow copy, since events may change, but events's items will not
      // change.
      mEvents = new HashMap<>();
      mEvents.putAll(events);
    } else {
      mEvents = null;
    }
  }

  @Override
  public void onClick(View widget) {}

  @Override
  public void updateDrawState(TextPaint ds) {}

  @Override
  public int getSign() {
    return mSign;
  }

  @Override
  public int getGestureArenaMemberId() {
    return 0;
  }

  @Override
  public int getPseudoStatus() {
    return mPseudoStatus;
  }

  @Override
  public EventTarget hitTest(float x, float y) {
    return hitTest(x, y, false);
  }

  @Override
  public EventTarget hitTest(float x, float y, boolean ignoreUserInteraction) {
    return this;
  }

  @Override
  public boolean isUserInteractionEnabled() {
    return true;
  }

  @Override
  public boolean containsPoint(float x, float y) {
    return true;
  }

  @Override
  public boolean containsPoint(float x, float y, boolean ignoreUserInteraction) {
    return true;
  }

  public void setParent(EventTarget target) {
    mParent = new WeakReference<>(target);
  }

  @Override
  public EventTarget parent() {
    return mParent.get();
  }

  @Override
  public EventTargetBase parentResponder() {
    if (mParent instanceof EventTargetBase) {
      return (EventTargetBase) mParent.get();
    }
    return null;
  }

  @Override
  public ReadableMap getDataset() {
    return mDataset;
  }

  @Override
  public Map<String, EventsListener> getEvents() {
    return mEvents;
  }

  @Override
  public Map<Integer, GestureDetector> getGestureDetectorMap() {
    return null;
  }

  @Override
  public Matrix getTransformMatrix() {
    mTransformMatrix.reset();
    return mTransformMatrix;
  }

  @Override
  public boolean ignoreFocus() {
    // If mIgnoreFocus == Enable, return true. If mIgnoreFocus == Disable, return false.
    // If mIgnoreFocus == Undefined && parent not null, return parent.ignoreFocus()
    if (mIgnoreFocus == EnableStatus.Enable) {
      return true;
    } else if (mIgnoreFocus == EnableStatus.Disable) {
      return false;
    } else if (parent() != null) {
      EventTarget parent = parent();
      // when parent is root ui, return false.
      if (parent instanceof UIBody) {
        return false;
      }
      return parent.ignoreFocus();
    }
    return false;
  }

  @Override
  public boolean isFocusable() {
    return false;
  }

  @Override
  public boolean isScrollable() {
    return false;
  }

  @Override
  public boolean isClickable() {
    return mEvents != null && mEvents.containsKey("tap");
  }

  @Override
  public boolean isLongClickable() {
    return mEvents != null && mEvents.containsKey("longpress");
  }

  @Override
  public boolean enableTouchPseudoPropagation() {
    return mEnableTouchPseudoPropagation;
  }

  @Override
  public void onPseudoStatusChanged(int preStatus, int currentStatus) {
    mPseudoStatus = currentStatus;
  }

  @Override
  public void onFocusChanged(boolean hasFocus, boolean isFocusTransition) {}

  @Override
  public void onResponseChain() {}

  @Override
  public void offResponseChain() {}

  @Override
  public boolean isOnResponseChain() {
    return false;
  }

  @Override
  public boolean consumeSlideEvent(float angle) {
    return false;
  }

  @Override
  public boolean hasConsumeSlideEventAngles() {
    return false;
  }

  // TODO(songshourui.null): return false now, will refactor in future.
  @Override
  public boolean blockNativeEvent(MotionEvent ev) {
    return false;
  }

  @Override
  public boolean dispatchTouch(MotionEvent ev) {
    return false;
  }

  @Override
  public boolean dispatchEvent(LynxEventDetail event) {
    return false;
  }

  @Override
  public boolean eventThrough() {
    if (mEventThrough == EventTarget.EnableStatus.Enable) {
      return true;
    } else if (mEventThrough == EventTarget.EnableStatus.Disable) {
      return false;
    }

    EventTarget parent = parent();
    if (parent != null) {
      return parent.eventThrough();
    }
    return false;
  }
}
