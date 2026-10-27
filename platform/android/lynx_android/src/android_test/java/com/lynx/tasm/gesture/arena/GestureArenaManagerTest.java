// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.arena;

import android.os.SystemClock;
import android.util.Log;
import android.view.MotionEvent;
import androidx.annotation.Nullable;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.event.LynxTouchEvent;
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
import org.mockito.ArgumentCaptor;
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

  @Test
  public void testUnifiedTouchInputKeepsPageAndWindowCoordinatesSeparate() throws Exception {
    RecordingNativeArena arena = new RecordingNativeArena();
    UIBody.UIBodyView view = Mockito.mock(UIBody.UIBodyView.class);
    Mockito.when(view.getRootView()).thenReturn(view);
    Mockito
        .doAnswer(invocation -> {
          int[] location = invocation.getArgument(0);
          location[0] = 100;
          location[1] = 200;
          return null;
        })
        .when(view)
        .getLocationOnScreen(Mockito.any(int[].class));
    mContext.setUIBodyView(view);
    setArenaField(arena, "mLynxContext", mContext);
    setArenaField(arena, "mNativeGestureArena", 1L);
    setArenaField(arena, "mUnifiedResponseChain", new int[] {9, 2});
    double density = mContext.getResources().getDisplayMetrics().density;
    long time = SystemClock.uptimeMillis();
    MotionEvent down = MotionEvent.obtain(time, time, MotionEvent.ACTION_DOWN, 131.5f, 241, 0);
    MotionEvent move = MotionEvent.obtain(time, time + 8, MotionEvent.ACTION_MOVE, 134.5f, 247, 0);
    MotionEvent up = MotionEvent.obtain(time, time + 16, MotionEvent.ACTION_UP, 138, 250, 0);
    MotionEvent cancel =
        MotionEvent.obtain(time, time + 24, MotionEvent.ACTION_CANCEL, 138, 250, 0);
    MotionEvent[] events = {down, move, up, down, cancel};
    try {
      for (MotionEvent event : events) {
        event.offsetLocation(-120, -220);
        arena.dispatchTouchEventToUnifiedArena(event);
        event.offsetLocation(120, 220);
      }
      Assert.assertEquals(Arrays.asList(0, 1, 2, 0, 3), arena.inputTypes);
      Assert.assertEquals(Arrays.asList(1L, 1L, 1L, 2L, 2L), arena.sequenceIds);
      Assert.assertArrayEquals(new double[] {131.5 / density, 241 / density, 11.5 / density,
                                   21 / density, 31.5 / density, 41 / density, 0, 0},
          arena.coordinates.get(0), 0.00001);
      Assert.assertArrayEquals(
          new double[] {134.5 / density, 247 / density, 14.5 / density, 27 / density,
              34.5 / density, 47 / density, -3 / density, -6 / density},
          arena.coordinates.get(1), 0.00001);
      Assert.assertArrayEquals(new int[] {9, 2}, arena.responseChains.get(0));
      Assert.assertNull(arena.responseChains.get(1));
      Assert.assertNull(arena.responseChains.get(2));
      Assert.assertArrayEquals(new int[] {9, 2}, arena.responseChains.get(3));
      Assert.assertNull(arena.responseChains.get(4));
    } finally {
      down.recycle();
      move.recycle();
      up.recycle();
      cancel.recycle();
      mContext.setUIBodyView(mLynxView);
    }
  }

  @Test
  public void testUnifiedMoveCallbackHonorsPlatformFilter() throws Exception {
    EventEmitter originalEmitter = mContext.getEventEmitter();
    EventEmitter emitter = Mockito.mock(EventEmitter.class);
    mContext.setEventEmitter(emitter);
    setArenaField(mGestureArenaManager, "mLynxContext", mContext);
    Method callback =
        arenaMethod("dispatchGestureEvent", int.class, int.class, int.class, int.class, long.class,
            double.class, double.class, double.class, double.class, double.class, double.class,
            double.class, double.class, double.class, double.class, boolean.class, boolean.class);
    try {
      callback.invoke(mGestureArenaManager, 1, 41, 1, 1, 42L, 1.5, 2.5, 11.5, 21.5, 31.5, 41.5,
          -3.0, -6.0, 7.0, 8.0, true, false);
      Mockito.verifyNoInteractions(emitter);

      setArenaField(mGestureArenaManager, "mUnifiedDispatchTouchMove", true);
      callback.invoke(mGestureArenaManager, 1, 41, 1, 1, 42L, 1.5, 2.5, 11.5, 21.5, 31.5, 41.5,
          -3.0, -6.0, 7.0, 8.0, true, false);
      ArgumentCaptor<LynxCustomEvent> captured = ArgumentCaptor.forClass(LynxCustomEvent.class);
      Mockito.verify(emitter).sendGestureEvent(Mockito.eq(41), captured.capture());
      LynxCustomEvent event = captured.getValue();
      Assert.assertEquals(GestureConstants.ON_TOUCHES_MOVE, event.getName());
      Map<String, Object> params = event.eventParams();
      Assert.assertEquals(42L, params.get("timestamp"));
      Assert.assertEquals(LynxTouchEvent.EVENT_TOUCH_MOVE, params.get("type"));
      Assert.assertEquals(1.5, params.get("x"));
      Assert.assertEquals(2.5, params.get("y"));
      Assert.assertEquals(11.5, params.get("pageX"));
      Assert.assertEquals(21.5, params.get("pageY"));
      Assert.assertEquals(31.5, params.get("clientX"));
      Assert.assertEquals(41.5, params.get("clientY"));
      Assert.assertEquals(-3.0, params.get("deltaX"));
      Assert.assertEquals(-6.0, params.get("deltaY"));
      Assert.assertEquals(7.0, params.get("scrollX"));
      Assert.assertEquals(8.0, params.get("scrollY"));
      Assert.assertEquals(true, params.get("isAtStart"));
      Assert.assertEquals(false, params.get("isAtEnd"));
    } finally {
      mContext.setEventEmitter(originalEmitter);
    }
  }

  @Test
  public void testUnifiedMemberScrollAndStateCallbacks() throws Exception {
    GestureArenaMember member = Mockito.mock(GestureArenaMember.class);
    Mockito.when(member.getSign()).thenReturn(41);
    Mockito.when(member.getMemberScrollX()).thenReturn(10, 16, 16);
    Mockito.when(member.getMemberScrollY()).thenReturn(20, 18, 18);
    Mockito.when(member.isAtBorder(true)).thenReturn(true);
    Mockito.when(member.getScrollContainerDirection())
        .thenReturn(GestureConstants.DIRECTION_VERTICAL);
    Mockito.when(member.canConsumeGesture(8.0f, -3.0f)).thenReturn(true);
    Mockito.when(member.shouldConsumeGesture()).thenReturn(true);
    setArenaField(mGestureArenaManager, "mLynxContext", mContext);
    Assert.assertEquals(41, mGestureArenaManager.addMember(member));
    double density = mContext.getResources().getDisplayMetrics().density;

    Assert.assertEquals(
        true, arenaMethod("isMemberValid", int.class).invoke(mGestureArenaManager, 41));
    Assert.assertEquals(
        false, arenaMethod("isMemberValid", int.class).invoke(mGestureArenaManager, 42));
    Assert.assertEquals(GestureConstants.DIRECTION_VERTICAL,
        arenaMethod("getScrollDirection", int.class).invoke(mGestureArenaManager, 41));
    Assert.assertEquals(
        true, arenaMethod("shouldConsumeGesture", int.class).invoke(mGestureArenaManager, 41));
    Assert.assertEquals(true,
        arenaMethod("canConsumeGesture", int.class, int.class, double.class, double.class)
            .invoke(mGestureArenaManager, 41, 2, 8 / density, -3 / density));
    Assert.assertArrayEquals(new double[] {6 / density, -2 / density, 2 / density, -1 / density},
        (double[]) arenaMethod("scrollBy", int.class, double.class, double.class)
            .invoke(mGestureArenaManager, 41, 8 / density, -3 / density),
        0.00001);
    Mockito.verify(member).onGestureScrollBy(8.0f, -3.0f);
    Assert.assertArrayEquals(new double[] {16 / density, 18 / density, 1, 0},
        (double[]) arenaMethod("getScrollState", int.class).invoke(mGestureArenaManager, 41),
        0.00001);
    Assert.assertArrayEquals(new double[] {1.5, 2.5},
        (double[]) arenaMethod("convertPageToMember", int.class, double.class, double.class)
            .invoke(mGestureArenaManager, 41, 1.5, 2.5),
        0);

    Method stateChanged = arenaMethod("onGestureStateChanged", int.class, int.class, int.class);
    stateChanged.invoke(mGestureArenaManager, 41, 1, 1);
    stateChanged.invoke(mGestureArenaManager, 41, 1, 2);
    stateChanged.invoke(mGestureArenaManager, 41, 1, 5);
    Mockito.verify(member).onPlatformGestureStatusChanged(GestureConstants.LYNX_STATE_BEGIN);
    Mockito.verify(member).onPlatformGestureStatusChanged(GestureConstants.LYNX_STATE_ACTIVE);
    Mockito.verify(member).onPlatformGestureStatusChanged(GestureConstants.LYNX_STATE_CANCELLED);
  }

  @Test
  public void testUnifiedJavaTimerCancellationSkipsNativeCallback() throws Exception {
    RecordingNativeArena arena = new RecordingNativeArena();
    setArenaField(arena, "mNativeGestureArena", 1L);
    Method schedule = arenaMethod("scheduleTimer", long.class, double.class);
    Method cancel = arenaMethod("cancelTimer", long.class);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      try {
        schedule.invoke(arena, 1L, 0.0);
        cancel.invoke(arena, 1L);
        schedule.invoke(arena, 2L, 0.0);
      } catch (ReflectiveOperationException e) {
        throw new AssertionError(e);
      }
    });
    InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    Assert.assertEquals(Collections.singletonList(2L), arena.timerTokens);
  }

  private static Method arenaMethod(String name, Class<?>... arguments) throws Exception {
    Method method = GestureArenaManager.class.getDeclaredMethod(name, arguments);
    method.setAccessible(true);
    return method;
  }

  private static void setArenaField(GestureArenaManager arena, String name, Object value)
      throws Exception {
    Field field = GestureArenaManager.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(arena, value);
  }

  private static class RecordingNativeArena extends GestureArenaManager {
    final List<Integer> inputTypes = new ArrayList<>();
    final List<Long> sequenceIds = new ArrayList<>();
    final List<double[]> coordinates = new ArrayList<>();
    final List<int[]> responseChains = new ArrayList<>();
    final List<Long> timerTokens = new ArrayList<>();

    @Override
    void nativeHandleInput(long nativePtr, int type, long sequenceId, int pointerId,
        double monotonicTime, long epochTime, double screenX, double screenY, double pageX,
        double pageY, double clientX, double clientY, double deltaX, double deltaY,
        double velocityX, double velocityY, boolean flingFinished, int[] responseChain) {
      inputTypes.add(type);
      sequenceIds.add(sequenceId);
      coordinates.add(
          new double[] {screenX, screenY, pageX, pageY, clientX, clientY, deltaX, deltaY});
      responseChains.add(responseChain);
    }

    @Override
    void nativeHandleTimer(long nativePtr, long token, double monotonicTime, long epochTime) {
      timerTokens.add(token);
    }
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
