// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.graphics.PointF;
import android.view.MotionEvent;

public interface IPaintingContext {
  // Keep in sync with native PlatformEventBehavior.
  int EVENT_BEHAVIOR_NONE = 0;
  int EVENT_BEHAVIOR_IGNORE_FOCUS = 1 << 0;
  int EVENT_BEHAVIOR_EVENT_THROUGH = 1 << 1;
  int EVENT_BEHAVIOR_BLOCK_NATIVE_EVENT = 1 << 2;
  int EVENT_BEHAVIOR_ENABLE_SIMULTANEOUS_TOUCH = 1 << 3;
  int EVENT_BEHAVIOR_HAS_CONSUME_SLIDE_EVENT = 1 << 4;

  // this func will be execed on main thread.
  void destroy();

  long getNativePaintingContextPtr();

  PointF convertPointInViewToScreen(int sign, PointF point);

  int getTargetWidth(int sign);

  int getTargetHeight(int sign);

  void setLynxEngineActorForPlatformContextRef(long ptr);

  boolean dispatchPlatformMotionEvent(MotionEvent ev, int rootSign);

  // Returns the hit target sign cached for the current pointer sequence.
  int getPlatformTouchTargetSign();

  // Returns event behavior cached for the current or next pointer sequence.
  int getPlatformEventBehavior();

  // Returns the angle intervals cached on the first pointer DOWN, as [min, max, ...].
  float[] getPlatformConsumeSlideEventAngles();

  // Hit-tests and caches event behavior for the next pointer sequence.
  int hitTestAndCachePlatformEventBehavior(int rootSign, float pointX, float pointY);

  void dispatchPlatformLongPress();

  void dispatchPlatformTap();

  void dispatchPlatformFocus();
}
