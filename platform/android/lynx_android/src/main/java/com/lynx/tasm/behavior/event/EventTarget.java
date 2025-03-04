// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.event;

import android.graphics.Matrix;
import android.view.MotionEvent;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxEventDetail;
import com.lynx.tasm.gesture.detector.GestureDetector;
import java.util.Map;

public interface EventTarget extends EventTargetBase {
  public enum EnableStatus { Enable, Disable, Undefined }

  int getSign();

  int getPseudoStatus();

  int getGestureArenaMemberId();

  EventTarget parent();

  EventTarget hitTest(float x, float y);
  EventTarget hitTest(float x, float y, boolean ignoreUserInteraction);

  boolean containsPoint(float x, float y);

  boolean containsPoint(float x, float y, boolean ignoreUserInteraction);

  Map<String, EventsListener> getEvents();

  Map<Integer, GestureDetector> getGestureDetectorMap();

  Matrix getTransformMatrix();

  boolean isUserInteractionEnabled();

  boolean ignoreFocus();

  boolean isFocusable();

  boolean isScrollable();

  boolean isClickable();

  boolean isLongClickable();

  boolean enableTouchPseudoPropagation();

  void onPseudoStatusChanged(int preStatus, int currentStatus);

  void onFocusChanged(boolean hasFocus, boolean isFocusTransition);

  void onResponseChain();

  void offResponseChain();

  boolean isOnResponseChain();

  boolean consumeSlideEvent(float angle);

  public boolean hasConsumeSlideEventAngles();

  boolean blockNativeEvent(MotionEvent ev);

  // Only care Android Touch, don't care Lynx frontend event, return value is not used now.
  boolean dispatchTouch(MotionEvent ev);

  // include both Android touch and Lynx frontend event, if return true, frontend event will not be
  // sent
  boolean dispatchEvent(LynxEventDetail event);

  boolean eventThrough();
}
