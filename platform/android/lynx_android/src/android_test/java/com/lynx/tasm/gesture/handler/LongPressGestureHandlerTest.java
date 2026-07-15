// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.handler;

import android.view.MotionEvent;
import androidx.annotation.Nullable;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.testing.base.TestingUtils;
import java.util.Map;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LongPressGestureHandlerTest {
  private LynxContext mContext;
  private LongPressGestureHandler mHandler;

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();
    LynxView lynxView = new LynxView(mContext, new LynxViewBuilder());
    mContext.setLynxView(lynxView);

    GestureDetector detector =
        new GestureDetector(1, GestureDetector.GESTURE_TYPE_LONG_PRESS, null, null);
    mHandler = new LongPressGestureHandler(1, mContext, detector, new GestureArenaMember() {
      @Override
      public void onGestureScrollBy(float deltaX, float deltaY) {}

      @Override
      public boolean canConsumeGesture(float deltaX, float deltaY) {
        return false;
      }

      @Override
      public int getSign() {
        return 1;
      }

      @Override
      public int getGestureArenaMemberId() {
        return 1;
      }

      @Override
      public int getMemberScrollX() {
        return 0;
      }

      @Override
      public int getScrollContainerDirection() {
        return GestureConstants.DIRECTION_UNDETERMINED;
      }

      @Override
      public boolean isAtBorder(boolean isStart) {
        return true;
      }

      @Override
      public int getMemberScrollY() {
        return 0;
      }

      @Override
      public void onInvalidate() {}

      @Override
      public void onPlatformGestureStatusChanged(int status) {}

      @Nullable
      @Override
      public Map<Integer, GestureDetector> getGestureDetectorMap() {
        return null;
      }

      @Nullable
      @Override
      public Map<Integer, BaseGestureHandler> getGestureHandlers() {
        return null;
      }
    });
  }

  @Test
  public void testMoveAfterActiveDoesNotFail() {
    LynxTouchEvent lynxTouchEvent = new LynxTouchEvent(1, "");
    MotionEvent down = MotionEvent.obtain(100, 100, MotionEvent.ACTION_DOWN, 0, 0, 0);
    mHandler.handleMotionEvent(down, lynxTouchEvent, 0, 0, false, null);
    down.recycle();
    Assert.assertEquals(GestureConstants.LYNX_STATE_BEGIN, mHandler.getGestureStatus());

    mHandler.activate();
    MotionEvent move = MotionEvent.obtain(100, 200, MotionEvent.ACTION_MOVE, 100, 100, 0);
    mHandler.handleMotionEvent(move, lynxTouchEvent, 0, 0, false, null);
    move.recycle();
    Assert.assertEquals(GestureConstants.LYNX_STATE_ACTIVE, mHandler.getGestureStatus());

    MotionEvent up = MotionEvent.obtain(100, 300, MotionEvent.ACTION_UP, 100, 100, 0);
    mHandler.handleMotionEvent(up, lynxTouchEvent, 0, 0, false, null);
    up.recycle();
  }
}
