// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.arena;

import android.util.Log;
import androidx.annotation.Nullable;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.LynxNewGestureDelegate;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.detector.GestureDetectorManager;
import com.lynx.tasm.gesture.handler.*;
import com.lynx.tasm.gesture.handler.GestureConstants;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.*;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;

/**
 * Test class for the GestureArenaManager, responsible for managing gesture arenas and members.
 */
@RunWith(AndroidJUnit4.class)
public class GestureArenaManagerTest {
  private static final String TAG = "GestureDetectorManagerTest";
  private LynxContext mContext;
  private LynxView mLynxView;
  private GestureArenaManager mGestureArenaManager;

  /**
   * Concrete implementation of GestureArenaMember used for testing purposes.
   * This class serves as a mock member for the gesture arena and provides test-specific behavior.
   */
  class ConcreteArenaMember implements GestureArenaMember {
    // Override methods from GestureArenaMember interface
    // These methods are used for handling gestures and managing the arena membership
    @Override
    public void onGestureScrollBy(float deltaX, float deltaY) {
      // No-op implementation for testing purposes
    }

    @Override
    public boolean canConsumeGesture(float deltaX, float deltaY) {
      return false;
    }

    @Override
    public boolean isAtBorder(boolean isStart) {
      return false;
    }

    @Override
    public int getScrollContainerDirection() {
      return GestureConstants.DIRECTION_UNDETERMINED;
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
  }

  private ConcreteArenaMember arenaMember1 = new ConcreteArenaMember() {
    // Implementation details specific to arenaMember1
    // This member has a gesture detector for WAIT_FOR relationship and a default gesture handler
    Map<String, List<Integer>> relationMap = new HashMap<>();
    GestureDetector detector = new GestureDetector(1, 1, null, relationMap);

    @Override
    public int getGestureArenaMemberId() {
      return 1;
    }

    @Override
    public int getSign() {
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
      handlers.put(GestureDetector.GESTURE_TYPE_DEFAULT,
          new DefaultGestureHandler(1, mContext, detector, this));
      return handlers;
    }
  };
  private ConcreteArenaMember arenaMember2 = new ConcreteArenaMember() {
    // Implementation details specific to arenaMember2
    // This member has a gesture detector for SIMULTANEOUS relationship and a pan gesture handler
    Map<String, List<Integer>> relationMap = new HashMap<>();
    GestureDetector detector = new GestureDetector(2, 1, null, relationMap);

    @Override
    public int getSign() {
      return 2;
    }

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
      handlers.put(
          GestureDetector.GESTURE_TYPE_PAN, new PanGestureHandler(2, mContext, detector, this));
      return handlers;
    }
  };

  ConcreteArenaMember arenaMember3 = new ConcreteArenaMember() {
    // Implementation details specific to arenaMember3
    // This member has a gesture detector for no specific relationship and a fling gesture handler
    GestureDetector detector = new GestureDetector(3, 1, null, null);

    @Override
    public int getSign() {
      return 3;
    }

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
      handlers.put(
          GestureDetector.GESTURE_TYPE_FLING, new FlingGestureHandler(3, mContext, detector, this));
      return handlers;
    }
  };

  @Before
  public void setUp() throws Exception {
    // Set up the testing environment before each test case
    // Initialize LynxContext, LynxView, GestureArenaManager, and add test members to the arena
    mContext = TestingUtils.getLynxContext();
    mLynxView = new LynxView(mContext, new LynxViewBuilder());
    mContext.setLynxView(mLynxView);
    mGestureArenaManager = new GestureArenaManager();
    JavaOnlyMap javaOnlyMap = Mockito.spy(JavaOnlyMap.class);
    javaOnlyMap.putBoolean("enableNewGesture", true);
    PageConfig config = new PageConfig(javaOnlyMap);
    mContext.onPageConfigDecoded(config);
    mGestureArenaManager.init(mContext.isEnableNewGesture(), mContext);
    mGestureArenaManager.addMember(arenaMember1);
    mGestureArenaManager.addMember(arenaMember2);
    mGestureArenaManager.addMember(arenaMember3);
  }

  @Test
  public void testInit() {
    // Test case for initializing GestureArenaManager
    // Ensure that the GestureDetectorManager is correctly initialized and not null
    GestureDetectorManager manager = getGestureDetectorManager();
    Assert.assertNotNull(manager);
  }

  @Test
  public void testLegacyArenaAndMemberDefaults() {
    Assert.assertFalse(mGestureArenaManager.isUsingUnifiedGestureHandler());
    Assert.assertTrue(arenaMember1.shouldConsumeGesture());
    mGestureArenaManager.replaceGestureDetectors(1, null);
    Assert.assertTrue(mGestureArenaManager.isMemberExist(1));
  }

  @Test
  public void testEncodeGestureDetectorsForNativeArena() {
    Map<String, List<Integer>> relations = new HashMap<>();
    relations.put(GestureDetector.SIMULTANEOUS, Arrays.asList(2, 4));
    relations.put(GestureDetector.WAIT_FOR, Collections.singletonList(6));
    relations.put(GestureDetector.CONTINUE_WITH, Collections.singletonList(8));
    JavaOnlyMap config = new JavaOnlyMap();
    config.putDouble(GestureConstants.MIN_DISTANCE, 1.5);
    config.putDouble(GestureConstants.MAX_DURATION, 250);
    List<String> callbacks = Arrays.asList(GestureConstants.ON_TOUCHES_DOWN,
        GestureConstants.ON_TOUCHES_MOVE, GestureConstants.ON_TOUCHES_UP,
        GestureConstants.ON_TOUCHES_CANCEL, GestureConstants.ON_BEGIN, GestureConstants.ON_START,
        GestureConstants.ON_UPDATE, GestureConstants.ON_END, "unrecognized");
    Map<Integer, GestureDetector> detectors = new LinkedHashMap<>();
    detectors.put(9, new GestureDetector(9, GestureDetector.GESTURE_TYPE_TAP, callbacks, null));
    detectors.put(3,
        new GestureDetector(3, GestureDetector.GESTURE_TYPE_PAN,
            Arrays.asList(GestureConstants.ON_TOUCHES_DOWN, GestureConstants.ON_END), relations,
            config));
    detectors.put(2, new GestureDetector(2, GestureDetector.GESTURE_TYPE_PINCH, null, null));
    detectors.put(4, null);

    GestureArenaManager.EncodedDetectors encoded =
        GestureArenaManager.encodeGestureDetectors(detectors);
    Assert.assertEquals(2, encoded.count);
    Assert.assertArrayEquals(new int[] {3, GestureDetector.GESTURE_TYPE_PAN, 129, 2, 1, 1, 2, 4, 6,
                                 8, 9, GestureDetector.GESTURE_TYPE_TAP, 255, 0, 0, 0},
        encoded.data);
    Assert.assertArrayEquals(
        new double[] {1.5, 10, 500, 250, 3, 0, 10, 500, 500, 3}, encoded.configs, 0);
  }

  @Test
  public void testEncodeNullGestureDetectorsClearsNativeRecords() {
    GestureArenaManager.EncodedDetectors encoded = GestureArenaManager.encodeGestureDetectors(null);
    Assert.assertEquals(0, encoded.count);
    Assert.assertArrayEquals(new int[0], encoded.data);
    Assert.assertArrayEquals(new double[0], encoded.configs, 0);
  }

  /**
   * Helper method to retrieve the GestureDetectorManager from GestureArenaManager using reflection.
   *
   * @return The retrieved GestureDetectorManager instance or null if not found.
   */
  private GestureDetectorManager getGestureDetectorManager() {
    // Reflectively retrieve the GestureDetectorManager instance from the GestureArenaManager
    // This is done using reflection to access a private member of the class for testing purposes
    GestureDetectorManager manager = null;
    try {
      Field field = mGestureArenaManager.getClass().getDeclaredField("mGestureDetectorManager");
      field.setAccessible(true);
      manager = (GestureDetectorManager) field.get(mGestureArenaManager);
    } catch (Exception e) {
      Log.e(TAG, e.toString());
    }
    return manager;
  }

  @Test
  public void testMember() {
    // Test case for managing members in the GestureArenaManager
    // Verify that members can be added to and removed from the gesture arena
    GestureDetectorManager manager = getGestureDetectorManager();
    Assert.assertNotNull(manager);
    Assert.assertTrue(mGestureArenaManager.isMemberExist(1));
    mGestureArenaManager.removeMember(arenaMember1);
  }

  @Test
  public void testSetGestureDetectors() {
    // Test case for setting gesture detectors for members
    // Ensure that gesture detectors are correctly set for the specified members
    try {
      Class<?> clazz = BaseGestureHandler.class;
      Method method = clazz.getDeclaredMethod("isActive");
      method.setAccessible(true);
      Assert.assertNotNull(
          method.invoke(mGestureArenaManager.getMemberById(2).getGestureHandlers().get(
              GestureDetector.GESTURE_TYPE_PAN)));
      boolean result =
          (boolean) method.invoke(mGestureArenaManager.getMemberById(2).getGestureHandlers().get(
                                      GestureDetector.GESTURE_TYPE_PAN),
              1.0f, 1.0f);
      mGestureArenaManager.setGestureDetectorState(2, 2, LynxNewGestureDelegate.STATE_FAIL);
      Assert.assertFalse(result);
    } catch (Exception e) {
      Log.e(TAG, e.toString());
    }
  }

  @Test
  public void testDestroy() {
    // Test case for destroying the GestureArenaManager
    // Verify that the arena is properly cleared when destroyed
    try {
      Field field = mGestureArenaManager.getClass().getDeclaredField("mArenaMemberMap");
      field.setAccessible(true);
      Map<Integer, GestureArenaMember> map =
          (Map<Integer, GestureArenaMember>) field.get(mGestureArenaManager);
      Assert.assertEquals(map.size(), 3);
      mGestureArenaManager.onDestroy();
      Assert.assertEquals(map.size(), 0);
    } catch (Exception e) {
      Log.e(TAG, e.toString());
    }
  }
}
