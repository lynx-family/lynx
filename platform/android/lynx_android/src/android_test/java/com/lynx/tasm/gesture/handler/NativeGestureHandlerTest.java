// Copyright 2024 The Lynx Authors
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
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test class for the NativeGestureHandler, responsible for handling native gestures in the
 * LynxView.
 */
@RunWith(AndroidJUnit4.class)
public class NativeGestureHandlerTest {
  private static final String TAG = "NativeGestureHandlerTest";
  private LynxContext mContext;
  private LynxView mLynxView;

  private GestureDetector mDetector;

  private NativeGestureHandler mHandler;

  private GestureArenaMember mGestureMember;

  @Before
  public void setUp() throws Exception {
    // Set up the testing environment before each test case
    // Initialize LynxContext, LynxView, GestureDetector, NativeGestureHandler, and
    // GestureArenaMember
    mContext = TestingUtils.getLynxContext();
    mLynxView = new LynxView(mContext, new LynxViewBuilder());
    mContext.setLynxView(mLynxView);

    Map<String, List<Integer>> map = new HashMap<>();
    List<String> gestureCallbackNames = new ArrayList<>();
    mDetector =
        new GestureDetector(1, GestureDetector.GESTURE_TYPE_NATIVE, gestureCallbackNames, map);
    mGestureMember = new GestureArenaMember() {
      @Override
      public void onGestureScrollBy(float deltaX, float deltaY) {}

      @Override
      public boolean canConsumeGesture(float deltaX, float deltaY) {
        return false;
      }

      @Override
      public boolean isAtBorder(boolean isStart) {
        return false;
      }

      @Override
      public int getSign() {
        return 0;
      }

      @Override
      public int getGestureArenaMemberId() {
        return 0;
      }

      @Override
      public int getMemberScrollX() {
        return 50;
      }

      @Override
      public int getMemberScrollY() {
        return 50;
      }

      @Override
      public void onInvalidate() {}

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
    };
    mHandler = new NativeGestureHandler(1, mContext, mDetector, mGestureMember);
  }

  @Test
  public void testCanActiveWithCurrentGesture() {
    // Test case for checking if the native gesture can be activated with the current gesture
    // Ensure that the handler correctly determines whether the native gesture can be activated
    Assert.assertTrue(mHandler.getGestureStatus() == GestureConstants.LYNX_STATE_INIT);
    mHandler.fail();
    Assert.assertFalse(mHandler.isActive());
  }

  @Test
  public void testGetEventParamsInActive() {
    // Test case for retrieving event parameters during the active state of the native gesture
    // Ensure that the handler returns the correct event parameters for the active state
    LynxTouchEvent event = new LynxTouchEvent(1, "");
    HashMap<String, Object> map = mHandler.getEventParamsInActive(event);
    mHandler.handleConfigMap(null);
    Assert.assertEquals(map.get("scrollX"), mHandler.px2dip(50));
    Assert.assertEquals(map.get("scrollY"), mHandler.px2dip(50));
  }

  @Test
  public void testOnHandle() {
    MotionEvent motionEvent = MotionEvent.obtain(100, 100, MotionEvent.ACTION_DOWN, 200, 200, 0);
    mHandler.onHandle(motionEvent, null, 200, 300);
    Assert.assertEquals(mHandler.getGestureStatus(), GestureConstants.LYNX_STATE_BEGIN);
    MotionEvent motionEvent2 = MotionEvent.obtain(100, 200, MotionEvent.ACTION_MOVE, 300, 300, 0);
    mHandler.onHandle(motionEvent2, null, 200, 300);
    Assert.assertEquals(mHandler.getGestureStatus(), GestureConstants.LYNX_STATE_ACTIVE);
    MotionEvent motionEvent3 = MotionEvent.obtain(100, 200, MotionEvent.ACTION_UP, 300, 300, 0);
    mHandler.onHandle(motionEvent3, null, 200, 300);
    Assert.assertEquals(mHandler.getGestureStatus(), GestureConstants.LYNX_STATE_FAIL);
  }

  @Test
  public void testEvents() {
    mHandler.onBegin(100, 100, null);
    mHandler.onUpdate(200, 200, null);
    mHandler.onStart(300, 300, null);
    mHandler.onEnd(400, 400, null);
  }
}
