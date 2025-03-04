// Copyright 2023 The Lynx Authors. All rights reserved.
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
import com.lynx.tasm.gesture.LynxNewGestureDelegate;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.detector.GestureDetectorManager;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Method;
import java.util.*;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test class for the GestureHandlerTrigger, responsible for triggering gestures and competing
 * members in the LynxView.
 */
@RunWith(AndroidJUnit4.class)
public class GestureHandlerTriggerTest {
  private GestureHandlerTrigger mHandlerTrigger;
  private static final String TAG = "GestureHandlerTriggerTest";
  private LynxContext mContext;
  private LynxView mLynxView;

  /**
   * Concrete implementation of GestureArenaMember used for testing purposes.
   * This class serves as a mock member for the gesture arena and provides test-specific behavior.
   */
  class ConcreteArenaMember implements GestureArenaMember {
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
      return 0;
    }

    @Override
    public int getMemberScrollY() {
      return 0;
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
  }

  private ConcreteArenaMember arenaMember1 = new ConcreteArenaMember() {
    // Implementation details specific to arenaMember1
    // This member has a gesture detector for WAIT_FOR relationship

    Map<String, List<Integer>> relationMap = new HashMap<>();
    GestureDetector detector = new GestureDetector(1, 1, null, relationMap);
    DefaultGestureHandler handler = new DefaultGestureHandler(1, mContext, detector, this);

    @Override
    public int getGestureArenaMemberId() {
      return 1;
    }

    @Nullable
    @Override
    public Map<Integer, GestureDetector> getGestureDetectorMap() {
      Map<Integer, GestureDetector> map = new HashMap<>();
      List<Integer> list = new ArrayList<>();
      list.add(2);
      relationMap.put(GestureDetector.WAIT_FOR, list);
      map.put(1, detector);
      return map;
    }

    @Nullable
    @Override
    public Map<Integer, BaseGestureHandler> getGestureHandlers() {
      Map<Integer, BaseGestureHandler> handlers = new HashMap<>();
      handlers.put(GestureDetector.GESTURE_TYPE_DEFAULT, handler);
      return handlers;
    }
  };
  private ConcreteArenaMember arenaMember2 = new ConcreteArenaMember() {
    // Implementation details specific to arenaMember2
    // This member has a gesture detector for SIMULTANEOUS relationship

    Map<String, List<Integer>> relationMap = new HashMap<>();
    GestureDetector detector = new GestureDetector(2, 1, null, relationMap);
    PanGestureHandler handler = new PanGestureHandler(2, mContext, detector, this);
    @Override
    public int getGestureArenaMemberId() {
      return 2;
    }

    @Nullable
    @Override
    public Map<Integer, GestureDetector> getGestureDetectorMap() {
      Map<Integer, GestureDetector> map = new HashMap<>();
      List<Integer> list = new ArrayList<>();
      list.add(3);
      relationMap.put(GestureDetector.SIMULTANEOUS, list);
      map.put(2, detector);
      return map;
    }

    @Nullable
    @Override
    public Map<Integer, BaseGestureHandler> getGestureHandlers() {
      Map<Integer, BaseGestureHandler> handlers = new HashMap<>();
      handlers.put(GestureDetector.GESTURE_TYPE_PAN, handler);
      return handlers;
    }
  };

  ConcreteArenaMember arenaMember3 = new ConcreteArenaMember() {
    // Implementation details specific to arenaMember3
    // This member has a standalone gesture detector

    GestureDetector detector = new GestureDetector(3, 1, null, null);
    FlingGestureHandler flingGestureHandler = new FlingGestureHandler(3, mContext, detector, this);
    @Override
    public int getGestureArenaMemberId() {
      return 3;
    }
    @Nullable
    @Override
    public Map<Integer, GestureDetector> getGestureDetectorMap() {
      Map<Integer, GestureDetector> map = new HashMap<>();
      map.put(3, detector);
      return map;
    }

    @Nullable
    @Override
    public Map<Integer, BaseGestureHandler> getGestureHandlers() {
      Map<Integer, BaseGestureHandler> handlers = new HashMap<>();
      handlers.put(GestureDetector.GESTURE_TYPE_FLING, flingGestureHandler);
      return handlers;
    }
  };
  @Before
  public void setUp() throws Exception {
    // Set up the testing environment before each test case
    // Initialize LynxContext, LynxView, GestureHandlerTrigger, and GestureArenaManager with
    // GestureDetectorManager

    mContext = TestingUtils.getLynxContext();
    mLynxView = new LynxView(mContext, new LynxViewBuilder());
    mContext.setLynxView(mLynxView);
    mHandlerTrigger =
        new GestureHandlerTrigger(mContext, new GestureDetectorManager(new GestureArenaManager()));
  }

  @Test
  public void testReCompeteByGesture() {
    // Test case for re-competing members based on the triggered gesture
    // Ensure that the handler correctly identifies the competing members and selects the winner

    try {
      Class<?> clazz = GestureHandlerTrigger.class;
      Method method = clazz.getDeclaredMethod(
          "reCompeteByGestures", LinkedList.class, GestureArenaMember.class);
      method.setAccessible(true);
      List<GestureArenaMember> list = new LinkedList<>();
      list.add(arenaMember1);
      list.add(arenaMember2);
      list.add(arenaMember3);
      GestureArenaMember result1 =
          (GestureArenaMember) method.invoke(mHandlerTrigger, list, arenaMember1);
      Assert.assertEquals(result1.getGestureArenaMemberId(), 1);

      // Retrieve the gesture handlers associated with the current member.
      Map<Integer, BaseGestureHandler> gestureHandler = arenaMember2.getGestureHandlers();

      // If there are no gesture handlers, return as there is no need to dispatch the event.
      if (gestureHandler == null) {
        return;
      }

      // Iterate through each gesture handler associated with the winner and handle the event.
      for (BaseGestureHandler handler : gestureHandler.values()) {
        handler.handleMotionEvent(null, null, 1, 1);
      }

      GestureArenaMember result2 =
          (GestureArenaMember) method.invoke(mHandlerTrigger, list, arenaMember2);
      Assert.assertEquals(result2.getGestureArenaMemberId(), 3);

      // Retrieve the gesture handlers associated with the current member.
      Map<Integer, BaseGestureHandler> gestureHandler3 = arenaMember3.getGestureHandlers();

      // If there are no gesture handlers, return as there is no need to dispatch the event.
      if (gestureHandler3 == null) {
        return;
      }

      // Iterate through each gesture handler associated with the winner and handle the event.
      for (BaseGestureHandler handler : gestureHandler3.values()) {
        handler.handleMotionEvent(MotionEvent.obtain(100, 1, MotionEvent.ACTION_DOWN, 1, 1, 0),
            new LynxTouchEvent(1, ""), 1, 1);
      }

      GestureArenaMember result3 =
          (GestureArenaMember) method.invoke(mHandlerTrigger, list, arenaMember3);
      Assert.assertEquals(result3.getGestureArenaMemberId(), 1);
      list.clear();
      list.add(arenaMember1);
      mHandlerTrigger.handleGestureDetectorState(
          arenaMember1, 1, LynxNewGestureDelegate.STATE_FAIL);
      GestureArenaMember result4 =
          (GestureArenaMember) method.invoke(mHandlerTrigger, list, arenaMember1);

      Assert.assertNull(result4);
    } catch (Exception e) {
      Assert.assertEquals("", e.toString());
    }
  }

  @Test
  public void testEnd() {
    try {
      Class<?> clazz = GestureHandlerTrigger.class;
      Method method = clazz.getDeclaredMethod(
          "reCompeteByGestures", LinkedList.class, GestureArenaMember.class);
      method.setAccessible(true);
      List<GestureArenaMember> list = new LinkedList<>();
      list.add(arenaMember1);
      list.add(arenaMember2);
      list.add(arenaMember3);
      GestureArenaMember result1 =
          (GestureArenaMember) method.invoke(mHandlerTrigger, list, arenaMember1);
      Assert.assertEquals(result1.getGestureArenaMemberId(), 1);
      mHandlerTrigger.handleGestureDetectorState(arenaMember2, 2, LynxNewGestureDelegate.STATE_END);
      Method methodCurrentMemberActive =
          clazz.getDeclaredMethod("getCurrentMemberState", GestureArenaMember.class);
      methodCurrentMemberActive.setAccessible(true);
      int state = (int) methodCurrentMemberActive.invoke(mHandlerTrigger, arenaMember2);
      Assert.assertEquals(state, GestureConstants.LYNX_STATE_END);
    } catch (Exception e) {
      Assert.assertEquals("", e.toString());
    }
  }
}
