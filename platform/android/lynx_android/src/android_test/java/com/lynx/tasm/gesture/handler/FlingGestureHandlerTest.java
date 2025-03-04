// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.handler;

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
 * Test class for the FlingGestureHandler, responsible for handling fling gestures in the LynxView.
 */
@RunWith(AndroidJUnit4.class)
public class FlingGestureHandlerTest {
  private static final String TAG = "PanGestureHandlerTest";
  private LynxContext mContext;
  private LynxView mLynxView;

  private GestureDetector mDetector;

  private FlingGestureHandler mHandler;

  private GestureArenaMember mGestureMember;

  @Before
  public void setUp() throws Exception {
    // Set up the testing environment before each test case
    // Initialize LynxContext, LynxView, GestureDetector, FlingGestureHandler, and
    // GestureArenaMember
    mContext = TestingUtils.getLynxContext();
    mLynxView = new LynxView(mContext, new LynxViewBuilder());
    mContext.setLynxView(mLynxView);

    Map<String, List<Integer>> map = new HashMap<>();
    List<String> gestureCallbackNames = new ArrayList<>();
    mDetector =
        new GestureDetector(1, GestureDetector.GESTURE_TYPE_FLING, gestureCallbackNames, map);
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
    mHandler = new FlingGestureHandler(1, mContext, mDetector, mGestureMember);
  }

  @Test
  public void testCanActiveWithCurrentGesture() {
    // Test case for checking if the fling gesture can be activated with the current gesture
    // Ensure that the handler correctly determines whether the fling gesture can be activated

    Assert.assertTrue(mHandler.getGestureStatus() == GestureConstants.LYNX_STATE_INIT);
    mHandler.fail();
    Assert.assertFalse(mHandler.isActive());
  }

  @Test
  public void testGetEventParamsInActive() {
    // Test case for retrieving event parameters during the active state of the fling gesture
    // Ensure that the handler returns the correct event parameters for the active state

    LynxTouchEvent event = new LynxTouchEvent(1, "");
    HashMap<String, Object> map = mHandler.getEventParamsInActive(1.0f, 1.0f);
    Assert.assertEquals(map.get("scrollX"), mHandler.px2dip(50));
    Assert.assertEquals(map.get("scrollY"), mHandler.px2dip(50));
    Assert.assertEquals((Integer) map.get("deltaX"), mHandler.px2dip(1.0f), 0.5f);
    Assert.assertEquals((Integer) map.get("deltaY"), mHandler.px2dip(1.0f), 0.5f);
  }
}
