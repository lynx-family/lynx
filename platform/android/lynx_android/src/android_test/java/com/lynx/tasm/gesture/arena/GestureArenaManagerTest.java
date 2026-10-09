// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.arena;

import android.app.Application;
import android.os.SystemClock;
import android.util.Log;
import android.view.MotionEvent;
import androidx.annotation.Nullable;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxEnv;
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
import com.lynx.tasm.service.ILynxTrailService;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.utils.MockLynxTrailService;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.*;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
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

  @BeforeClass
  public static void loadNativeLibrary() {
    System.loadLibrary("lynx");
  }

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

  class RecordingArenaMember extends ConcreteArenaMember {
    private final int mMemberId;
    private Map<Integer, GestureDetector> mDetectors;
    private final List<Integer> mStates = Collections.synchronizedList(new ArrayList<>());

    RecordingArenaMember(int memberId, Map<Integer, GestureDetector> detectors) {
      mMemberId = memberId;
      mDetectors = detectors;
    }

    @Override
    public int getSign() {
      return mMemberId;
    }

    @Override
    public int getGestureArenaMemberId() {
      return mMemberId;
    }

    @Override
    public Map<Integer, GestureDetector> getGestureDetectorMap() {
      return mDetectors;
    }

    @Override
    public void onPlatformGestureStatusChanged(int status) {
      mStates.add(status);
    }

    void setDetectors(Map<Integer, GestureDetector> detectors) {
      mDetectors = detectors;
    }

    List<Integer> getStates() {
      return mStates;
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
  public void testUnifiedInit() {
    mGestureArenaManager.onDestroy();
    mGestureArenaManager.init(true, true, mContext);
    try {
      Assert.assertTrue(mGestureArenaManager.isUsingUnifiedGestureHandler());
      Assert.assertNull(getGestureDetectorManager());

      ConcreteArenaMember unsupportedMember = new ConcreteArenaMember() {
        @Override
        public int getSign() {
          return 4;
        }

        @Override
        public int getGestureArenaMemberId() {
          return 4;
        }

        @Override
        public Map<Integer, GestureDetector> getGestureDetectorMap() {
          Map<Integer, GestureDetector> detectors = new HashMap<>();
          detectors.put(
              4, new GestureDetector(4, GestureDetector.GESTURE_TYPE_ROTATION, null, null));
          return detectors;
        }
      };
      Assert.assertEquals(0, mGestureArenaManager.addMember(unsupportedMember));
      Assert.assertFalse(mGestureArenaManager.isMemberExist(4));
    } finally {
      mGestureArenaManager.onDestroy();
    }
  }

  @Test
  public void testUnifiedPageConfigDefaultAndOverride() {
    Assert.assertTrue(new PageConfig(new JavaOnlyMap()).isEnableUnifiedGestureHandler());

    JavaOnlyMap configMap = new JavaOnlyMap();
    configMap.putBoolean("enableUnifiedGestureHandler", false);
    Assert.assertFalse(new PageConfig(configMap).isEnableUnifiedGestureHandler());
  }

  @Test
  public void testUnifiedGlobalSettingIsReadForEachNewArena() {
    Application application = (Application) InstrumentationRegistry.getInstrumentation()
                                  .getTargetContext()
                                  .getApplicationContext();
    Map<String, Object> settings = new HashMap<>();
    settings.put("enable_unified_gesture_handler", "true");
    LynxServiceCenter.inst().initialize(application);
    LynxServiceCenter.inst().registerService(new MockLynxTrailService(settings));
    LynxEnv.inst().setSettings(new HashMap<>());

    JavaOnlyMap pageConfigMap = new JavaOnlyMap();
    pageConfigMap.putBoolean("enableUnifiedGestureHandler", true);
    PageConfig pageConfig = new PageConfig(pageConfigMap);
    JavaOnlyMap disabledPageConfigMap = new JavaOnlyMap();
    disabledPageConfigMap.putBoolean("enableUnifiedGestureHandler", false);
    PageConfig disabledPageConfig = new PageConfig(disabledPageConfigMap);
    GestureArenaManager firstPageManager = new GestureArenaManager();
    GestureArenaManager secondPageManager = new GestureArenaManager();
    GestureArenaManager thirdPageManager = new GestureArenaManager();
    GestureArenaManager fourthPageManager = new GestureArenaManager();
    try {
      firstPageManager.init(true, pageConfig.isEnableUnifiedGestureHandler(), mContext);
      Assert.assertTrue(firstPageManager.isUsingUnifiedGestureHandler());

      settings.put("enable_unified_gesture_handler", "false");
      LynxEnv.inst().setSettings(new HashMap<>());
      Assert.assertTrue(firstPageManager.isUsingUnifiedGestureHandler());

      secondPageManager.init(true, pageConfig.isEnableUnifiedGestureHandler(), mContext);
      Assert.assertFalse(secondPageManager.isUsingUnifiedGestureHandler());
      Assert.assertNotNull(getGestureDetectorManager(secondPageManager));

      thirdPageManager.init(true, disabledPageConfig.isEnableUnifiedGestureHandler(), mContext);
      Assert.assertFalse(thirdPageManager.isUsingUnifiedGestureHandler());
      Assert.assertNotNull(getGestureDetectorManager(thirdPageManager));

      settings.put("enable_unified_gesture_handler", "true");
      LynxEnv.inst().setSettings(new HashMap<>());
      fourthPageManager.init(true, disabledPageConfig.isEnableUnifiedGestureHandler(), mContext);
      Assert.assertFalse(fourthPageManager.isUsingUnifiedGestureHandler());
      Assert.assertNotNull(getGestureDetectorManager(fourthPageManager));
    } finally {
      firstPageManager.onDestroy();
      secondPageManager.onDestroy();
      thirdPageManager.onDestroy();
      fourthPageManager.onDestroy();
      LynxServiceCenter.inst().unregisterService(ILynxTrailService.class);
      LynxEnv.inst().setSettings(new HashMap<>());
    }
  }

  @Test
  public void testUnifiedGestureReplacementAndClearReachNative() throws Exception {
    mGestureArenaManager.onDestroy();
    mGestureArenaManager.init(true, true, mContext);
    Map<Integer, GestureDetector> initialDetectors = new HashMap<>();
    initialDetectors.put(20, new GestureDetector(20, GestureDetector.GESTURE_TYPE_TAP, null, null));
    RecordingArenaMember member = new RecordingArenaMember(20, initialDetectors);
    Assert.assertEquals(20, mGestureArenaManager.addMember(member));
    setUnifiedResponseChain(20);

    long downTime = SystemClock.uptimeMillis();
    MotionEvent down = MotionEvent.obtain(downTime, downTime, MotionEvent.ACTION_DOWN, 8, 9, 0);
    try {
      mGestureArenaManager.dispatchTouchEventToArena(down, null);
      Assert.assertEquals(Arrays.asList(GestureConstants.LYNX_STATE_BEGIN), member.getStates());

      Map<Integer, GestureDetector> replacement = new HashMap<>();
      replacement.put(
          21, new GestureDetector(21, GestureDetector.GESTURE_TYPE_LONG_PRESS, null, null));
      member.setDetectors(replacement);
      mGestureArenaManager.replaceGestureDetectors(20, replacement);
      Assert.assertEquals(
          Arrays.asList(GestureConstants.LYNX_STATE_BEGIN, GestureConstants.LYNX_STATE_CANCELLED),
          member.getStates());

      mGestureArenaManager.setGestureDetectorState(20, 20, LynxNewGestureDelegate.STATE_FAIL);
      Assert.assertEquals(2, member.getStates().size());
      mGestureArenaManager.setGestureDetectorState(20, 21, LynxNewGestureDelegate.STATE_FAIL);
      Assert.assertEquals(GestureConstants.LYNX_STATE_FAIL,
          (int) member.getStates().get(member.getStates().size() - 1));

      member.setDetectors(Collections.emptyMap());
      mGestureArenaManager.replaceGestureDetectors(20, Collections.emptyMap());
      int stateCountAfterClear = member.getStates().size();
      mGestureArenaManager.setGestureDetectorState(20, 21, LynxNewGestureDelegate.STATE_END);
      Assert.assertEquals(stateCountAfterClear, member.getStates().size());
    } finally {
      down.recycle();
      mGestureArenaManager.onDestroy();
    }
  }

  @Test
  public void testUnifiedDuplicateTypeKeepsMinimumGestureId() throws Exception {
    mGestureArenaManager.onDestroy();
    mGestureArenaManager.init(true, true, mContext);
    Map<Integer, GestureDetector> detectors = new HashMap<>();
    detectors.put(31, new GestureDetector(31, GestureDetector.GESTURE_TYPE_PAN, null, null));
    detectors.put(30, new GestureDetector(30, GestureDetector.GESTURE_TYPE_PAN, null, null));
    RecordingArenaMember member = new RecordingArenaMember(30, detectors);
    Assert.assertEquals(30, mGestureArenaManager.addMember(member));
    setUnifiedResponseChain(30);

    long downTime = SystemClock.uptimeMillis();
    MotionEvent down = MotionEvent.obtain(downTime, downTime, MotionEvent.ACTION_DOWN, 4, 5, 0);
    try {
      mGestureArenaManager.dispatchTouchEventToArena(down, null);
      Assert.assertEquals(Arrays.asList(GestureConstants.LYNX_STATE_BEGIN), member.getStates());

      mGestureArenaManager.setGestureDetectorState(30, 31, LynxNewGestureDelegate.STATE_FAIL);
      Assert.assertEquals(1, member.getStates().size());
      mGestureArenaManager.setGestureDetectorState(30, 30, LynxNewGestureDelegate.STATE_FAIL);
      Assert.assertEquals(
          Arrays.asList(GestureConstants.LYNX_STATE_BEGIN, GestureConstants.LYNX_STATE_FAIL),
          member.getStates());
    } finally {
      down.recycle();
      mGestureArenaManager.onDestroy();
    }
  }

  @Test
  public void testUnifiedCancelPreventsLongPressTimer() throws Exception {
    mGestureArenaManager.onDestroy();
    mGestureArenaManager.init(true, true, mContext);
    JavaOnlyMap config = new JavaOnlyMap();
    config.putDouble(GestureConstants.MIN_DURATION, 50);
    Map<Integer, GestureDetector> detectors = new HashMap<>();
    detectors.put(
        40, new GestureDetector(40, GestureDetector.GESTURE_TYPE_LONG_PRESS, null, null, config));
    RecordingArenaMember member = new RecordingArenaMember(40, detectors);
    Assert.assertEquals(40, mGestureArenaManager.addMember(member));
    setUnifiedResponseChain(40);

    long downTime = SystemClock.uptimeMillis();
    MotionEvent down = MotionEvent.obtain(downTime, downTime, MotionEvent.ACTION_DOWN, 6, 7, 0);
    MotionEvent cancel =
        MotionEvent.obtain(downTime, downTime + 1, MotionEvent.ACTION_CANCEL, 6, 7, 0);
    try {
      mGestureArenaManager.dispatchTouchEventToArena(down, null);
      mGestureArenaManager.dispatchTouchEventToArena(cancel, null);
      SystemClock.sleep(100);
      InstrumentationRegistry.getInstrumentation().waitForIdleSync();
      Assert.assertEquals(
          Arrays.asList(GestureConstants.LYNX_STATE_BEGIN, GestureConstants.LYNX_STATE_CANCELLED),
          member.getStates());
      Assert.assertFalse(mGestureArenaManager.hasActivePlatformGesture());
    } finally {
      down.recycle();
      cancel.recycle();
      mGestureArenaManager.onDestroy();
    }
  }

  @Test
  public void testUnifiedInputAndCancelReachNative() {
    mGestureArenaManager.onDestroy();
    mGestureArenaManager.init(true, true, mContext);
    ConcreteArenaMember member = new ConcreteArenaMember() {
      private final GestureDetector detector =
          new GestureDetector(5, GestureDetector.GESTURE_TYPE_TAP, null, null);

      @Override
      public int getSign() {
        return 5;
      }

      @Override
      public int getGestureArenaMemberId() {
        return 5;
      }

      @Override
      public Map<Integer, GestureDetector> getGestureDetectorMap() {
        Map<Integer, GestureDetector> detectors = new HashMap<>();
        detectors.put(5, detector);
        return detectors;
      }
    };

    long downTime = SystemClock.uptimeMillis();
    MotionEvent down = MotionEvent.obtain(downTime, downTime, MotionEvent.ACTION_DOWN, 12, 18, 0);
    MotionEvent move =
        MotionEvent.obtain(downTime, downTime + 1, MotionEvent.ACTION_MOVE, 13, 19, 0);
    MotionEvent cancel =
        MotionEvent.obtain(downTime, downTime + 2, MotionEvent.ACTION_CANCEL, 13, 19, 0);
    try {
      Assert.assertEquals(5, mGestureArenaManager.addMember(member));
      mGestureArenaManager.dispatchTouchEventToArena(down, null);
      mGestureArenaManager.dispatchTouchEventToArena(move, null);
      mGestureArenaManager.dispatchTouchEventToArena(cancel, null);
      Assert.assertTrue(mGestureArenaManager.isMemberExist(5));
    } finally {
      down.recycle();
      move.recycle();
      cancel.recycle();
      mGestureArenaManager.onDestroy();
    }
  }

  /**
   * Helper method to retrieve the GestureDetectorManager from GestureArenaManager using reflection.
   *
   * @return The retrieved GestureDetectorManager instance or null if not found.
   */
  private GestureDetectorManager getGestureDetectorManager() {
    return getGestureDetectorManager(mGestureArenaManager);
  }

  private GestureDetectorManager getGestureDetectorManager(GestureArenaManager arenaManager) {
    // Reflectively retrieve the GestureDetectorManager instance from the GestureArenaManager
    // This is done using reflection to access a private member of the class for testing purposes
    GestureDetectorManager manager = null;
    try {
      Field field = arenaManager.getClass().getDeclaredField("mGestureDetectorManager");
      field.setAccessible(true);
      manager = (GestureDetectorManager) field.get(arenaManager);
    } catch (Exception e) {
      Log.e(TAG, e.toString());
    }
    return manager;
  }

  private void setUnifiedResponseChain(int... memberIds) throws Exception {
    Field field = mGestureArenaManager.getClass().getDeclaredField("mUnifiedResponseChain");
    field.setAccessible(true);
    field.set(mGestureArenaManager, memberIds);
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
