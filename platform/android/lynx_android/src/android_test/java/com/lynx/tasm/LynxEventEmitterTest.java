// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import androidx.annotation.NonNull;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.core.LynxEngineProxy;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.event.LynxEvent;
import com.lynx.tasm.event.LynxInternalEvent;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.utils.UIThreadUtils;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.CountDownLatch;
import org.junit.Before;
import org.junit.Test;

public class LynxEventEmitterTest {
  LynxEventEmitter mEventEmitter;

  LynxTouchEvent mTouchEvent = null;
  LynxCustomEvent mCustomEvent = null;

  int mGestureTag = -1;
  int mGestureID = -1;
  int mGestureLength = -1;
  String mGestureName = null;
  ByteBuffer mGestureBuffer = null;

  int mPseudoID = -1;
  int mPseudoPreStatus = -1;
  int mPseudoCurrentStatus = -1;

  boolean mLynxEventReporterReturn = false;
  LynxEvent mLynxEvent = null;
  LynxInternalEvent mLynxInternalEvent = null;

  private int mTapCount = 0;

  class MockEventReporter implements EventEmitter.LynxEventReporter {
    public boolean onLynxEvent(LynxEvent event) {
      mLynxEvent = event;
      return mLynxEventReporterReturn;
    }

    public void onInternalEvent(@NonNull LynxInternalEvent event) {
      mLynxInternalEvent = event;
    }
  }

  class MockEventFallback implements EventEmitter.LynxEventFallback {
    @Override
    public void checkFallbackForLynxEvent(boolean isFallback) {}
  }

  class MockEventObserver implements EventEmitter.LynxEventObserver {
    int eventCount;
    EventEmitter.LynxEventType lastType;
    LynxEvent lastEvent;
    boolean calledOnUiThread;

    @Override
    public void onLynxEvent(EventEmitter.LynxEventType type, LynxEvent event) {
      eventCount++;
      lastType = type;
      lastEvent = event;
      calledOnUiThread = UIThreadUtils.isOnUiThread();
    }
  }

  class MockEngineProxyWrapper extends LynxEventEmitter.LynxEngineProxyWrapper {
    MockEngineProxyWrapper(LynxEngineProxy proxy) {
      super(proxy);
    }

    @Override
    void sendTouchEvent(LynxTouchEvent event) {
      mTouchEvent = event;
    }

    @Override
    void sendMultiTouchEvent(LynxTouchEvent event) {
      mTouchEvent = event;
    }

    @Override
    void sendCustomEvent(LynxCustomEvent event) {
      mCustomEvent = event;
    }

    @Override
    void sendGestureEvent(final String name, final int tag, int gestureId, final ByteBuffer params,
        final int length) {
      mGestureName = name;
      mGestureTag = tag;
      mGestureID = gestureId;
      mGestureBuffer = params;
      mGestureLength = length;
    }

    @Override
    void onPseudoStatusChanged(final int id, final int preStatus, final int currentStatus) {
      mPseudoID = id;
      mPseudoPreStatus = preStatus;
      mPseudoCurrentStatus = currentStatus;
    }
  }

  class MockTestTapTrack implements EventEmitter.ITestTapTrack {
    @Override
    public void onTap() {
      mTapCount++;
    }
  }

  @Before
  public void setUp() {
    mEventEmitter = new LynxEventEmitter(null);
    mEventEmitter.mEngineProxy = new MockEngineProxyWrapper(null);
    mEventEmitter.setTestTapTracker(new MockTestTapTrack());
    mEventEmitter.registerEventReporter(new MockEventReporter());
    mEventEmitter.registerEventFallback(new MockEventFallback());

    reset();
  }

  private void reset() {
    mTouchEvent = null;
    mCustomEvent = null;

    mGestureTag = -1;
    mGestureID = -1;
    mGestureLength = -1;
    mGestureName = null;
    mGestureBuffer = null;

    mPseudoID = -1;
    mPseudoPreStatus = -1;
    mPseudoCurrentStatus = -1;

    mLynxEventReporterReturn = false;
    mLynxEvent = null;
    mLynxInternalEvent = null;

    mTapCount = 0;
  }

  private void waitMainThread() {
    final CountDownLatch latch = new CountDownLatch(1);
    if (!UIThreadUtils.isOnUiThread()) {
      UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
        @Override
        public void run() {
          latch.countDown();
        }
      });
    }

    try {
      latch.await();
    } catch (Throwable e) {
      fail(e.toString());
    }
  }

  @Test
  public void testSendTouchEvent() {
    LynxTouchEvent event = new LynxTouchEvent(11, "xxx", 12, 13);
    mEventEmitter.mEngineProxy = null;
    mEventEmitter.sendTouchEvent(event);
    assertNull(mTouchEvent);

    mLynxEventReporterReturn = true;
    mEventEmitter.mEngineProxy = new MockEngineProxyWrapper(null);
    mEventEmitter.sendTouchEvent(event);
    assertEquals(mLynxEvent, event);
    assertEquals(mTapCount, 0);
    assertNull(mTouchEvent);

    mLynxEventReporterReturn = false;
    mEventEmitter.sendTouchEvent(event);
    assertEquals(mLynxEvent, event);
    assertEquals(mTapCount, 0);
    assertEquals(mTouchEvent, event);

    reset();

    LynxTouchEvent tapEvent = new LynxTouchEvent(11, "tap", 12, 13);
    mEventEmitter.mEngineProxy = null;
    mEventEmitter.sendTouchEvent(tapEvent);
    assertNull(mTouchEvent);

    mLynxEventReporterReturn = true;
    mEventEmitter.mEngineProxy = new MockEngineProxyWrapper(null);
    mEventEmitter.sendTouchEvent(tapEvent);
    assertEquals(mLynxEvent, tapEvent);
    assertEquals(mTapCount, 0);
    assertNull(mTouchEvent);

    mLynxEventReporterReturn = false;
    mEventEmitter.sendTouchEvent(tapEvent);
    assertEquals(mLynxEvent, tapEvent);
    assertEquals(mTapCount, 1);
    assertEquals(mTouchEvent, tapEvent);
  }

  @Test
  public void testTouchEventSerializesCurrentTargetPoints() {
    LynxTouchEvent event = new LynxTouchEvent(11, "tap", 12, 13);
    ArrayList<Object> legacyDetail = (ArrayList<Object>) event.getEventParams().get(5);
    assertEquals(7, legacyDetail.size());

    HashMap<Integer, LynxTouchEvent.Point> points = new HashMap<>();
    points.put(11, new LynxTouchEvent.Point(7.5f, 8.5f));
    event.setCurrentTargetPointMap(points);

    ArrayList<Object> detail = (ArrayList<Object>) event.getEventParams().get(5);
    ArrayList<Object> serializedPoints = (ArrayList<Object>) detail.get(7);
    ArrayList<Object> point = (ArrayList<Object>) serializedPoints.get(0);

    assertEquals(11, point.get(0));
    assertEquals(7.5f, (Float) point.get(1), 0.0f);
    assertEquals(8.5f, (Float) point.get(2), 0.0f);
  }

  @Test
  public void testSendMultiTouchEvent() {
    LynxTouchEvent event = new LynxTouchEvent(11, "xxx", 12, 13);
    mEventEmitter.mEngineProxy = null;
    mEventEmitter.sendMultiTouchEvent(event);
    assertNull(mTouchEvent);

    mLynxEventReporterReturn = true;
    mEventEmitter.mEngineProxy = new MockEngineProxyWrapper(null);
    mEventEmitter.sendMultiTouchEvent(event);
    assertEquals(mLynxEvent, event);
    assertEquals(mTapCount, 0);
    assertNull(mTouchEvent);

    mLynxEventReporterReturn = false;
    mEventEmitter.sendMultiTouchEvent(event);
    assertEquals(mLynxEvent, event);
    assertEquals(mTapCount, 0);
    assertEquals(mTouchEvent, event);

    reset();

    LynxTouchEvent tapEvent = new LynxTouchEvent(11, "tap", 12, 13);
    mEventEmitter.mEngineProxy = null;
    mEventEmitter.sendMultiTouchEvent(tapEvent);
    assertNull(mTouchEvent);

    mLynxEventReporterReturn = true;
    mEventEmitter.mEngineProxy = new MockEngineProxyWrapper(null);
    mEventEmitter.sendMultiTouchEvent(tapEvent);
    assertEquals(mLynxEvent, tapEvent);
    assertEquals(mTapCount, 0);
    assertNull(mTouchEvent);

    mLynxEventReporterReturn = false;
    mEventEmitter.sendMultiTouchEvent(tapEvent);
    assertEquals(mLynxEvent, tapEvent);
    assertEquals(mTapCount, 0);
    assertEquals(mTouchEvent, tapEvent);
  }

  @Test
  public void testSendCustomEvent() {
    HashMap<String, Object> params = new HashMap<>();
    params.put("1", "1");
    params.put("2", "2");

    LynxCustomEvent event = new LynxCustomEvent(11, "xxx", params);
    mEventEmitter.mEngineProxy = null;
    mEventEmitter.sendCustomEvent(event);
    assertNull(mCustomEvent);

    mLynxEventReporterReturn = true;
    mEventEmitter.mEngineProxy = new MockEngineProxyWrapper(null);
    mEventEmitter.sendCustomEvent(event);
    waitMainThread();
    assertEquals(mLynxEvent, event);
    assertNull(mCustomEvent);

    mLynxEventReporterReturn = false;
    mEventEmitter.sendCustomEvent(event);
    waitMainThread();
    assertEquals(mLynxEvent, event);
    assertEquals(mCustomEvent, event);
  }

  @Test
  public void testSendGestureEvent() {
    HashMap<String, Object> params = new HashMap<>();
    params.put("1", "1");
    params.put("2", "2");

    LynxCustomEvent event = new LynxCustomEvent(11, "xxx", params);
    mEventEmitter.mEngineProxy = null;
    mEventEmitter.sendGestureEvent(11, event);
    assertEquals(mGestureID, -1);
    assertEquals(mGestureTag, -1);
    assertEquals(mGestureLength, -1);
    assertNull(mGestureName);
    assertNull(mGestureBuffer);

    mEventEmitter.mEngineProxy = new MockEngineProxyWrapper(null);
    mEventEmitter.sendGestureEvent(11, event);
    assertEquals(mGestureTag, 11);
    assertEquals(mGestureID, 11);
    assertEquals(mGestureName, "xxx");

    ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(event.eventParams());
    assertEquals(mGestureLength, buffer.position());
    assertEquals(mGestureBuffer, buffer);
  }

  @Test
  public void testOnPseudoStatusChanged() {
    mEventEmitter.mEngineProxy = null;
    mEventEmitter.onPseudoStatusChanged(11, 1, 1);
    assertEquals(mPseudoPreStatus, -1);
    assertEquals(mPseudoCurrentStatus, -1);

    mEventEmitter.mEngineProxy = new MockEngineProxyWrapper(null);
    mEventEmitter.onPseudoStatusChanged(11, 1, 1);
    assertEquals(mPseudoPreStatus, -1);
    assertEquals(mPseudoCurrentStatus, -1);

    mEventEmitter.onPseudoStatusChanged(11, 1, 2);
    assertEquals(mPseudoPreStatus, 1);
    assertEquals(mPseudoCurrentStatus, 2);
  }

  @Test
  public void testUiAppearObserverRegistrationAndRemoval() {
    MockEventObserver observer = new MockEventObserver();
    LynxCustomEvent event = new LynxCustomEvent(11, "uiappear");

    assertTrue(UIThreadUtils.runOnUiThreadSync(() -> {
      mEventEmitter.addObserver(observer);
      mEventEmitter.addObserver(observer);
      mEventEmitter.sendCustomEvent(event);
      assertEquals(1, observer.eventCount);
      assertEquals(EventEmitter.LynxEventType.kLynxEventTypeCustomEvent, observer.lastType);
      assertSame(event, observer.lastEvent);
      assertSame(event, mCustomEvent);
      assertTrue(observer.calledOnUiThread);

      mEventEmitter.removeObserver(observer);
      mEventEmitter.sendCustomEvent(new LynxCustomEvent(11, "uiappear"));
      assertEquals(1, observer.eventCount);
      assertEquals("uiappear", mCustomEvent.getName());
    }));
  }

  @Test
  public void testLayoutObserverRunsOnUiThread() {
    MockEventObserver observer = new MockEventObserver();
    assertTrue(UIThreadUtils.runOnUiThreadSync(() -> mEventEmitter.addObserver(observer)));

    mEventEmitter.sendLayoutEvent();
    assertTrue(UIThreadUtils.runOnUiThreadSync(() -> {
      assertEquals(1, observer.eventCount);
      assertEquals(EventEmitter.LynxEventType.kLynxEventTypeLayoutEvent, observer.lastType);
      assertNull(observer.lastEvent);
      assertTrue(observer.calledOnUiThread);
    }));
  }

  @Test
  public void testObserverCanRemoveItselfDuringUiAppear() {
    MockEventObserver firstObserver = new MockEventObserver() {
      @Override
      public void onLynxEvent(EventEmitter.LynxEventType type, LynxEvent event) {
        super.onLynxEvent(type, event);
        mEventEmitter.removeObserver(this);
      }
    };
    MockEventObserver secondObserver = new MockEventObserver();

    assertTrue(UIThreadUtils.runOnUiThreadSync(() -> {
      mEventEmitter.addObserver(firstObserver);
      mEventEmitter.addObserver(secondObserver);
      mEventEmitter.sendCustomEvent(new LynxCustomEvent(11, "uiappear"));
      assertEquals(1, firstObserver.eventCount);
      assertEquals(1, secondObserver.eventCount);

      mEventEmitter.sendCustomEvent(new LynxCustomEvent(11, "uiappear"));
      assertEquals(1, firstObserver.eventCount);
      assertEquals(2, secondObserver.eventCount);
    }));
  }

  @Test
  public void testObserverPreservesCustomEventDispatchConditions() {
    MockEventObserver observer = new MockEventObserver();
    LynxCustomEvent event = new LynxCustomEvent(11, "uiappear");

    assertTrue(UIThreadUtils.runOnUiThreadSync(() -> {
      mEventEmitter.addObserver(observer);
      mLynxEventReporterReturn = true;
      mEventEmitter.sendCustomEvent(event);
      assertEquals(0, observer.eventCount);
      assertNull(mCustomEvent);

      mLynxEventReporterReturn = false;
      mEventEmitter.setInPreLoad(true);
      mEventEmitter.sendCustomEvent(event);
      assertEquals(1, observer.eventCount);
      assertSame(event, observer.lastEvent);
      assertNull(mCustomEvent);
    }));
  }
}
