// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.handler;

import static com.lynx.tasm.gesture.handler.GestureConstants.DIRECTION_VERTICAL;
import static com.lynx.tasm.gesture.handler.GestureConstants.LYNX_STATE_ACTIVE;
import static com.lynx.tasm.gesture.handler.GestureConstants.LYNX_STATE_BEGIN;
import static com.lynx.tasm.gesture.handler.GestureConstants.LYNX_STATE_CANCELLED;
import static com.lynx.tasm.gesture.handler.GestureConstants.LYNX_STATE_END;
import static com.lynx.tasm.gesture.handler.GestureConstants.LYNX_STATE_FAIL;
import static com.lynx.tasm.gesture.handler.GestureConstants.LYNX_STATE_INIT;
import static com.lynx.tasm.gesture.handler.GestureConstants.LYNX_STATE_UNDETERMINED;
import static com.lynx.tasm.gesture.handler.GestureConstants.MAX_DISTANCE;
import static com.lynx.tasm.gesture.handler.GestureConstants.MAX_DURATION;
import static com.lynx.tasm.gesture.handler.GestureConstants.MIN_DISTANCE;
import static com.lynx.tasm.gesture.handler.GestureConstants.MIN_DURATION;
import static com.lynx.tasm.gesture.handler.GestureConstants.ON_BEGIN;
import static com.lynx.tasm.gesture.handler.GestureConstants.ON_END;
import static com.lynx.tasm.gesture.handler.GestureConstants.ON_START;
import static com.lynx.tasm.gesture.handler.GestureConstants.ON_UPDATE;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyFloat;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.view.MotionEvent;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.gesture.common.GestureExtraBundle;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.detector.GestureDetectorManager;
import com.lynx.tasm.utils.PixelUtils;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class GestureHandlerBaselineTest {
  private LynxContext mContext;
  private GestureArenaMember mMember;
  private final List<String> mEvents = new ArrayList<>();
  private final List<Map<String, Object>> mParams = new ArrayList<>();
  private final List<BaseGestureHandler> mHandlers = new ArrayList<>();

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();
    mMember = mock(GestureArenaMember.class);
    when(mMember.getSign()).thenReturn(10);
    when(mMember.getGestureArenaMemberId()).thenReturn(10);
    when(mMember.canConsumeGesture(anyFloat(), anyFloat())).thenReturn(true);
    when(mMember.getScrollContainerDirection()).thenReturn(DIRECTION_VERTICAL);
    EventEmitter emitter = mock(EventEmitter.class);
    doAnswer(invocation -> {
      LynxCustomEvent event = invocation.getArgument(1);
      mEvents.add(event.getName());
      mParams.add(new HashMap<>(event.eventParams()));
      return null;
    })
        .when(emitter)
        .sendGestureEvent(anyInt(), any(LynxCustomEvent.class));
    mContext.setEventEmitter(emitter);
  }

  @After
  public void tearDown() {
    onMain(() -> {
      for (BaseGestureHandler handler : mHandlers) {
        send(handler, MotionEvent.ACTION_UP, 0, 0);
        handler.handleMotionEvent(null, null, Float.MIN_VALUE, Float.MIN_VALUE, false, null);
      }
    });
  }

  private void onMain(Runnable action) {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(action);
  }

  private BaseGestureHandler handler(int type, JavaOnlyMap config, String... callbacks) {
    GestureDetector detector = new GestureDetector(7, type, Arrays.asList(callbacks), null, config);
    BaseGestureHandler result =
        BaseGestureHandler
            .convertToGestureHandler(10, mContext, mMember, Collections.singletonMap(7, detector))
            .get(type);
    assertNotNull(result);
    mHandlers.add(result);
    return result;
  }

  private BaseGestureHandler handler(int type) {
    return handler(type, null, ON_BEGIN, ON_START, ON_UPDATE, ON_END);
  }

  private void send(BaseGestureHandler handler, int action, float x, float y) {
    MotionEvent event = MotionEvent.obtain(100, 110, action, x, y, 0);
    try {
      handler.handleMotionEvent(event, new LynxTouchEvent(10, "touch"), 0, 0, false, null);
    } finally {
      event.recycle();
    }
  }

  @Test
  public void panAndNativeReleasePreserveCallbackOrder() {
    onMain(() -> {
      for (int type :
          new int[] {GestureDetector.GESTURE_TYPE_PAN, GestureDetector.GESTURE_TYPE_NATIVE}) {
        mEvents.clear();
        BaseGestureHandler pan = handler(type);
        send(pan, MotionEvent.ACTION_DOWN, 0, 0);
        send(pan, MotionEvent.ACTION_MOVE, 20, 30);
        send(pan, MotionEvent.ACTION_MOVE, 25, 35);
        send(pan, MotionEvent.ACTION_UP, 25, 35);
        assertEquals(Arrays.asList(ON_BEGIN, ON_START, ON_UPDATE, ON_UPDATE, ON_END), mEvents);
        assertEquals(LYNX_STATE_FAIL, pan.getGestureStatus());
      }
    });
  }

  @Test
  public void panThresholdIsStrictAndAxisBasedInBothDirections() {
    onMain(() -> {
      JavaOnlyMap config = new JavaOnlyMap();
      config.putLong(MIN_DISTANCE, 10);
      float threshold = PixelUtils.dipToPx(10, 0);
      for (int direction : new int[] {-1, 1}) {
        BaseGestureHandler pan = handler(GestureDetector.GESTURE_TYPE_PAN, config);
        send(pan, MotionEvent.ACTION_DOWN, 0, 0);
        send(pan, MotionEvent.ACTION_MOVE, direction * threshold, direction * threshold);
        assertEquals(LYNX_STATE_BEGIN, pan.getGestureStatus());
        send(pan, MotionEvent.ACTION_MOVE, direction * (threshold + 1), 0);
        assertEquals(LYNX_STATE_ACTIVE, pan.getGestureStatus());
      }
    });
  }

  @Test
  public void panWithoutBeginSubscriptionStillStartsAndEnds() {
    onMain(() -> {
      BaseGestureHandler pan = handler(GestureDetector.GESTURE_TYPE_PAN, null, ON_START, ON_END);
      send(pan, MotionEvent.ACTION_DOWN, 0, 0);
      send(pan, MotionEvent.ACTION_MOVE, 1, 0);
      send(pan, MotionEvent.ACTION_UP, 1, 0);
      assertEquals(Arrays.asList(ON_START, ON_END), mEvents);
    });
  }

  @Test
  public void panReleaseBeforeActivationUsesCancelledState() {
    onMain(() -> {
      BaseGestureHandler pan = handler(GestureDetector.GESTURE_TYPE_PAN);
      send(pan, MotionEvent.ACTION_DOWN, 0, 0);
      send(pan, MotionEvent.ACTION_UP, 0, 0);
      assertEquals(LYNX_STATE_CANCELLED, pan.getGestureStatus());
      assertEquals(Arrays.asList(ON_BEGIN, ON_END), mEvents);
    });
  }

  @Test
  public void panHandlerCancelDoesNotTerminateUntilTriggerActs() {
    onMain(() -> {
      BaseGestureHandler pan = handler(GestureDetector.GESTURE_TYPE_PAN);
      send(pan, MotionEvent.ACTION_DOWN, 0, 0);
      send(pan, MotionEvent.ACTION_CANCEL, 0, 0);
      assertEquals(LYNX_STATE_BEGIN, pan.getGestureStatus());
      assertEquals(Collections.singletonList(ON_BEGIN), mEvents);
    });
  }

  @Test
  public void explicitEndDominatesFailAndEndCallbackIsDeduplicated() {
    onMain(() -> {
      BaseGestureHandler pan = handler(GestureDetector.GESTURE_TYPE_PAN);
      send(pan, MotionEvent.ACTION_DOWN, 0, 0);
      pan.end();
      pan.fail();
      pan.end();
      send(pan, MotionEvent.ACTION_MOVE, 20, 0);
      assertEquals(LYNX_STATE_END, pan.getGestureStatus());
      assertEquals(Arrays.asList(ON_BEGIN, ON_END), mEvents);
    });
  }

  @Test
  public void resetAllowsAnotherCompletePanSequence() {
    onMain(() -> {
      BaseGestureHandler pan = handler(GestureDetector.GESTURE_TYPE_PAN);
      for (int i = 0; i < 2; ++i) {
        pan.reset();
        send(pan, MotionEvent.ACTION_DOWN, 0, 0);
        send(pan, MotionEvent.ACTION_MOVE, 1, 0);
        send(pan, MotionEvent.ACTION_UP, 1, 0);
      }
      assertEquals(Arrays.asList(ON_BEGIN, ON_START, ON_UPDATE, ON_END, ON_BEGIN, ON_START,
                       ON_UPDATE, ON_END),
          mEvents);
    });
  }

  @Test
  public void tapAtDistanceBoundarySucceedsButBeyondFails() {
    onMain(() -> {
      JavaOnlyMap config = new JavaOnlyMap();
      config.putLong(MAX_DISTANCE, 10);
      config.putLong(MAX_DURATION, 60000);
      float threshold = PixelUtils.dipToPx(10);
      for (int direction : new int[] {-1, 1}) {
        for (int beyond : new int[] {0, 1}) {
          mEvents.clear();
          BaseGestureHandler tap =
              handler(GestureDetector.GESTURE_TYPE_TAP, config, ON_BEGIN, ON_START, ON_END);
          send(tap, MotionEvent.ACTION_DOWN, 0, 0);
          send(tap, MotionEvent.ACTION_MOVE, direction * (threshold + beyond), 0);
          send(tap, MotionEvent.ACTION_UP, direction * (threshold + beyond), 0);
          assertEquals(beyond == 0 ? Arrays.asList(ON_BEGIN, ON_START, ON_END)
                                   : Arrays.asList(ON_BEGIN, ON_END),
              mEvents);
        }
      }
    });
  }

  @Test
  public void tapTimeoutPreventsSubsequentClick() {
    JavaOnlyMap config = new JavaOnlyMap();
    config.putLong(MAX_DURATION, 0);
    BaseGestureHandler tap =
        handler(GestureDetector.GESTURE_TYPE_TAP, config, ON_BEGIN, ON_START, ON_END);
    onMain(() -> send(tap, MotionEvent.ACTION_DOWN, 0, 0));
    InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    onMain(() -> {
      send(tap, MotionEvent.ACTION_UP, 0, 0);
      assertEquals(Arrays.asList(ON_BEGIN, ON_END), mEvents);
      assertEquals(LYNX_STATE_CANCELLED, tap.getGestureStatus());
    });
  }

  @Test
  public void longPressRemainsActiveBeyondDistanceAfterActivation() {
    JavaOnlyMap config = new JavaOnlyMap();
    config.putLong(MIN_DURATION, 0);
    BaseGestureHandler press =
        handler(GestureDetector.GESTURE_TYPE_LONG_PRESS, config, ON_BEGIN, ON_START, ON_END);
    onMain(() -> send(press, MotionEvent.ACTION_DOWN, 0, 0));
    InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    onMain(() -> {
      assertEquals(LYNX_STATE_ACTIVE, press.getGestureStatus());
      send(press, MotionEvent.ACTION_MOVE, 1000, -1000);
      assertEquals(LYNX_STATE_ACTIVE, press.getGestureStatus());
      send(press, MotionEvent.ACTION_UP, 1000, -1000);
      assertEquals(Arrays.asList(ON_BEGIN, ON_START, ON_END), mEvents);
    });
  }

  @Test
  public void longPressReleasedInSameTurnCancelsPendingActivation() {
    JavaOnlyMap config = new JavaOnlyMap();
    config.putLong(MIN_DURATION, 0);
    BaseGestureHandler press =
        handler(GestureDetector.GESTURE_TYPE_LONG_PRESS, config, ON_BEGIN, ON_START, ON_END);
    onMain(() -> {
      send(press, MotionEvent.ACTION_DOWN, 0, 0);
      send(press, MotionEvent.ACTION_UP, 0, 0);
    });
    InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    assertEquals(Arrays.asList(ON_BEGIN, ON_END), mEvents);
  }

  @Test
  public void flingAfterReleaseUpdatesWithoutStartCallback() {
    onMain(() -> {
      BaseGestureHandler fling = handler(GestureDetector.GESTURE_TYPE_FLING);
      send(fling, MotionEvent.ACTION_DOWN, 0, 0);
      assertEquals(LYNX_STATE_UNDETERMINED, fling.getGestureStatus());
      send(fling, MotionEvent.ACTION_UP, 0, 0);
      fling.handleMotionEvent(null, null, 5, -6, false, null);
      assertEquals(LYNX_STATE_BEGIN, fling.getGestureStatus());
      assertEquals(Arrays.asList(ON_BEGIN, ON_UPDATE), mEvents);
      fling.handleMotionEvent(null, null, Float.MIN_VALUE, Float.MIN_VALUE, false, null);
      assertEquals(Arrays.asList(ON_BEGIN, ON_UPDATE, ON_END), mEvents);
    });
  }

  @Test
  public void flingWithoutReleaseConsumesFirstFrameAsActivation() {
    onMain(() -> {
      BaseGestureHandler fling = handler(GestureDetector.GESTURE_TYPE_FLING);
      fling.handleMotionEvent(null, null, 5, -6, false, null);
      assertEquals(LYNX_STATE_ACTIVE, fling.getGestureStatus());
      assertEquals(Arrays.asList(ON_BEGIN, ON_START), mEvents);
      fling.handleMotionEvent(null, null, 7, -8, false, null);
      assertEquals(Arrays.asList(ON_BEGIN, ON_START, ON_UPDATE), mEvents);
      assertFalse(mParams.get(2).containsKey("timestamp"));
      assertFalse(mParams.get(2).containsKey("pageX"));
    });
  }

  @Test
  public void failedSimultaneousDefaultStillConsumesSharedDelta() {
    onMain(() -> {
      BaseGestureHandler scroll = handler(GestureDetector.GESTURE_TYPE_DEFAULT);
      scroll.fail();
      GestureExtraBundle bundle = new GestureExtraBundle();
      bundle.setNeedConsumedSimultaneousGesture(true);
      bundle.setSimultaneousDeltaX(4.5f);
      bundle.setSimultaneousDeltaY(-7.5f);
      scroll.handleMotionEvent(null, null, 99, 99, true, bundle);
      verify(mMember).onGestureScrollBy(4.5f, -7.5f);
      assertEquals(LYNX_STATE_FAIL, scroll.getGestureStatus());
      assertTrue(mEvents.isEmpty());
    });
  }

  @Test
  public void defaultLocksDirectionAndPublishesTheConsumedDelta() {
    onMain(() -> {
      BaseGestureHandler scroll = handler(GestureDetector.GESTURE_TYPE_DEFAULT);
      send(scroll, MotionEvent.ACTION_DOWN, 0, 0);
      GestureExtraBundle bundle = new GestureExtraBundle();
      MotionEvent move = MotionEvent.obtain(100, 120, MotionEvent.ACTION_MOVE, 4, 4, 0);
      try {
        scroll.handleMotionEvent(move, null, 0, 0, false, bundle);
      } finally {
        move.recycle();
      }
      assertEquals(DIRECTION_VERTICAL, bundle.getGestureDirection());
      verify(mMember).onGestureScrollBy(0, -4);
      assertEquals(0, bundle.getSimultaneousDeltaX(), 0);
      assertEquals(-4, bundle.getSimultaneousDeltaY(), 0);
      bundle.resetSimultaneousDelta();
      assertEquals(DIRECTION_VERTICAL, bundle.getGestureDirection());
      assertFalse(bundle.isNeedConsumedSimultaneousGesture());
    });
  }

  @Test
  public void factoryCollapsesDuplicateTypesAndIgnoresUnsupportedTypes() {
    Map<Integer, GestureDetector> detectors = new LinkedHashMap<>();
    detectors.put(8, new GestureDetector(8, GestureDetector.GESTURE_TYPE_PAN, null, null));
    detectors.put(3, new GestureDetector(3, GestureDetector.GESTURE_TYPE_PAN, null, null));
    detectors.put(9, new GestureDetector(9, GestureDetector.GESTURE_TYPE_PINCH, null, null));
    Map<Integer, BaseGestureHandler> handlers =
        BaseGestureHandler.convertToGestureHandler(10, mContext, mMember, detectors);
    assertEquals(1, handlers.size());
    assertEquals(
        3, handlers.get(GestureDetector.GESTURE_TYPE_PAN).getGestureDetector().getGestureID());
  }

  @Test
  public void failedParentCanReenterAfterChildFails() throws Exception {
    GestureArenaMember child = mock(GestureArenaMember.class);
    when(child.getGestureArenaMemberId()).thenReturn(11);
    BaseGestureHandler parentPan = handler(GestureDetector.GESTURE_TYPE_PAN);
    BaseGestureHandler childPan =
        BaseGestureHandler
            .convertToGestureHandler(11, mContext, child,
                Collections.singletonMap(
                    8, new GestureDetector(8, GestureDetector.GESTURE_TYPE_NATIVE, null, null)))
            .get(GestureDetector.GESTURE_TYPE_NATIVE);
    when(mMember.getGestureHandlers()).thenReturn(Collections.singletonMap(0, parentPan));
    when(child.getGestureHandlers()).thenReturn(Collections.singletonMap(7, childPan));
    GestureHandlerTrigger trigger =
        new GestureHandlerTrigger(mContext, new GestureDetectorManager(new GestureArenaManager()));
    try {
      Method compete = GestureHandlerTrigger.class.getDeclaredMethod(
          "reCompeteByGestures", LinkedList.class, GestureArenaMember.class);
      compete.setAccessible(true);
      LinkedList<GestureArenaMember> chain = new LinkedList<>(Arrays.asList(mMember, child));
      parentPan.fail();
      assertSame(child, compete.invoke(trigger, chain, mMember));
      childPan.fail();
      assertSame(mMember, compete.invoke(trigger, chain, child));
      assertEquals(LYNX_STATE_INIT, parentPan.getGestureStatus());
    } finally {
      trigger.onDestroy();
    }
  }

  @Test
  public void panReleaseDoesNotDiscardSameNodeFling() throws Exception {
    BaseGestureHandler pan = handler(GestureDetector.GESTURE_TYPE_PAN);
    BaseGestureHandler fling =
        BaseGestureHandler
            .convertToGestureHandler(10, mContext, mMember,
                Collections.singletonMap(8,
                    new GestureDetector(8, GestureDetector.GESTURE_TYPE_FLING,
                        Arrays.asList(ON_BEGIN, ON_START, ON_UPDATE, ON_END), null)))
            .get(GestureDetector.GESTURE_TYPE_FLING);
    Map<Integer, BaseGestureHandler> handlers = new HashMap<>();
    handlers.put(GestureDetector.GESTURE_TYPE_PAN, pan);
    handlers.put(GestureDetector.GESTURE_TYPE_FLING, fling);
    when(mMember.getGestureHandlers()).thenReturn(handlers);
    GestureHandlerTrigger trigger =
        new GestureHandlerTrigger(mContext, new GestureDetectorManager(new GestureArenaManager()));
    try {
      trigger.initCurrentWinnerWhenDown(mMember);
      send(pan, MotionEvent.ACTION_DOWN, 0, 0);
      send(fling, MotionEvent.ACTION_DOWN, 0, 0);
      send(pan, MotionEvent.ACTION_MOVE, 20, 0);
      send(pan, MotionEvent.ACTION_UP, 20, 0);
      send(fling, MotionEvent.ACTION_UP, 20, 0);
      Method state = GestureHandlerTrigger.class.getDeclaredMethod(
          "getCurrentMemberState", GestureArenaMember.class);
      state.setAccessible(true);
      assertEquals(LYNX_STATE_BEGIN, state.invoke(trigger, mMember));
      mEvents.clear();
      fling.handleMotionEvent(null, null, 8, -9, false, null);
      assertEquals(Collections.singletonList(ON_UPDATE), mEvents);
    } finally {
      trigger.onDestroy();
    }
  }

  @Test
  public void waitForOrdersLaterCandidatesAndTruncatesAncestors() {
    GestureDetectorManager manager = new GestureDetectorManager(new GestureArenaManager());
    GestureArenaMember second = mock(GestureArenaMember.class);
    GestureArenaMember third = mock(GestureArenaMember.class);
    GestureArenaMember ancestor = mock(GestureArenaMember.class);
    when(second.getGestureArenaMemberId()).thenReturn(20);
    when(third.getGestureArenaMemberId()).thenReturn(30);
    when(ancestor.getGestureArenaMemberId()).thenReturn(40);
    GestureDetector firstDetector = new GestureDetector(1, GestureDetector.GESTURE_TYPE_PAN, null,
        Collections.singletonMap("waitFor", Arrays.asList(3, 2)));
    GestureDetector secondDetector =
        new GestureDetector(2, GestureDetector.GESTURE_TYPE_PAN, null, null);
    GestureDetector thirdDetector =
        new GestureDetector(3, GestureDetector.GESTURE_TYPE_PAN, null, null);
    when(mMember.getGestureDetectorMap()).thenReturn(Collections.singletonMap(1, firstDetector));
    when(second.getGestureDetectorMap()).thenReturn(Collections.singletonMap(2, secondDetector));
    when(third.getGestureDetectorMap()).thenReturn(Collections.singletonMap(3, thirdDetector));
    manager.registerGestureDetector(10, firstDetector);
    manager.registerGestureDetector(20, secondDetector);
    manager.registerGestureDetector(30, thirdDetector);
    assertEquals(Arrays.asList(third, second, mMember),
        manager.convertResponseChainToCompeteChain(
            new LinkedList<>(Arrays.asList(mMember, second, third, ancestor))));
  }

  @Test
  public void touchPayloadRoundsSignedCoordinatesAndUsesUnixMilliseconds() {
    BaseGestureHandler pan = handler(GestureDetector.GESTURE_TYPE_PAN);
    float density = mContext.getResources().getDisplayMetrics().density;
    LynxTouchEvent touch = new LynxTouchEvent(10, "touchstart",
        new LynxTouchEvent.Point(100.25f * density, -200.5f * density),
        new LynxTouchEvent.Point(30.5f * density, -40.25f * density),
        new LynxTouchEvent.Point(-1.25f * density, 2.75f * density));
    long before = System.currentTimeMillis();
    pan.onTouchesDown(touch);
    assertTrue(mEvents.isEmpty());
    MotionEvent down = MotionEvent.obtain(100, 110, MotionEvent.ACTION_DOWN, 0, 0, 0);
    try {
      pan.handleMotionEvent(down, touch, 0, 0, false, null);
    } finally {
      down.recycle();
    }
    Map<String, Object> params = mParams.get(0);
    assertEquals(0, ((Number) params.get("x")).intValue());
    assertEquals(3, ((Number) params.get("y")).intValue());
    assertEquals(31, ((Number) params.get("pageX")).intValue());
    assertEquals(-39, ((Number) params.get("pageY")).intValue());
    assertEquals("touchstart", params.get("type"));
    assertTrue(((Number) params.get("timestamp")).longValue() >= before);
    assertTrue(((Number) params.get("timestamp")).longValue() <= System.currentTimeMillis());
  }
}
