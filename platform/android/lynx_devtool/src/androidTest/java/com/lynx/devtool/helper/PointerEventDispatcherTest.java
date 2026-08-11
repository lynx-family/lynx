// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.res.Resources;
import android.os.IBinder;
import android.util.DisplayMetrics;
import android.view.MotionEvent;
import android.view.View;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxView;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class PointerEventDispatcherTest {
  private static final float DENSITY = 2.f;
  private static final int ROOT_WIDTH = 800;
  private static final int ROOT_HEIGHT = 1200;

  private LynxView mLynxView;
  private CapturingRootView mWindowRoot;
  private PointerEventDispatcher mDispatcher;
  private List<CapturedEvent> mEvents;

  @Before
  public void setUp() {
    mLynxView = mock(LynxView.class);
    IBinder windowToken = mock(IBinder.class);
    mWindowRoot = createWindowRoot(windowToken);
    configureWindow(mLynxView, mWindowRoot, windowToken);
    mEvents = mWindowRoot.getCapturedEvents();
    mDispatcher = new PointerEventDispatcher(mLynxView);
  }

  @Test
  public void injectsCoordinatesOutsideLynxViewThroughWindowRoot() {
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 300.f, 400.f, 0.f, 0.f, 7, 0, 1000));
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_UP, 300.f, 400.f, 0.f, 0.f, 7, 0, 2000));

    assertEquals(2, mEvents.size());
    assertEquals(MotionEvent.ACTION_DOWN, mEvents.get(0).action);
    assertEquals(600.f, mEvents.get(0).x, 0.f);
    assertEquals(800.f, mEvents.get(0).y, 0.f);
    assertEquals(7, mEvents.get(0).pointerId);
    assertEquals(mEvents.get(0).downTime, mEvents.get(1).downTime);
    assertEquals(MotionEvent.ACTION_UP, mEvents.get(1).action);
    verify(mLynxView, never()).dispatchTouchEvent(any(MotionEvent.class));
  }

  @Test
  public void rejectsCoordinatesOutsideWindowBounds() {
    assertFalse(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, -1.f, 20.f, 0.f, 0.f, 7, 0, 1000));
    assertFalse(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 400.f, 20.f, 0.f, 0.f, 7, 0, 1000));
    assertFalse(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 20.f, 600.f, 0.f, 0.f, 7, 0, 1000));

    assertTrue(mEvents.isEmpty());
  }

  @Test
  public void rejectsRepeatedDownAndPreservesActivePointer() {
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 1000));
    assertFalse(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 20.f, 30.f, 0.f, 0.f, 8, 0, 2000));
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_UP, 10.f, 20.f, 0.f, 0.f, 7, 0, 3000));

    assertEquals(2, mEvents.size());
    assertEquals(MotionEvent.ACTION_DOWN, mEvents.get(0).action);
    assertEquals(MotionEvent.ACTION_UP, mEvents.get(1).action);
  }

  @Test
  public void rejectsWrongPointerIdAndAcceptsOriginalPointer() {
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 1000));
    assertFalse(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_UP, 20.f, 30.f, 0.f, 0.f, 8, 0, 2000));
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_UP, 10.f, 20.f, 0.f, 0.f, 7, 0, 4000));

    assertEquals(2, mEvents.size());
    assertEquals(7, mEvents.get(0).pointerId);
    assertEquals(7, mEvents.get(1).pointerId);
  }

  @Test
  public void detachCancelsActiveSequence() {
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 1000));

    mDispatcher.detach();

    assertEquals(2, mEvents.size());
    assertEquals(MotionEvent.ACTION_CANCEL, mEvents.get(1).action);
    assertEquals(7, mEvents.get(1).pointerId);
    assertFalse(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 8, 0, 2000));
  }

  @Test
  public void attachCancelsOldWindowAndUsesCurrentWindow() {
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 1000));

    LynxView nextLynxView = mock(LynxView.class);
    IBinder nextWindowToken = mock(IBinder.class);
    CapturingRootView nextWindowRoot = createWindowRoot(nextWindowToken);
    configureWindow(nextLynxView, nextWindowRoot, nextWindowToken);
    List<CapturedEvent> nextEvents = nextWindowRoot.getCapturedEvents();
    mDispatcher.attach(nextLynxView);

    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 30.f, 40.f, 0.f, 0.f, 8, 0, 2000));
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_UP, 30.f, 40.f, 0.f, 0.f, 8, 0, 3000));

    assertEquals(2, mEvents.size());
    assertEquals(MotionEvent.ACTION_CANCEL, mEvents.get(1).action);
    assertEquals(2, nextEvents.size());
    assertEquals(8, nextEvents.get(0).pointerId);
  }

  @Test
  public void returnsWindowDispatchResult() {
    mWindowRoot.setDispatchResult(false);

    assertFalse(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 1000));

    mWindowRoot.setDispatchResult(true);
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 2000));
  }

  @Test
  public void rejectsDetachedLynxView() {
    when(mLynxView.getWindowToken()).thenReturn(null);

    assertFalse(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 1000));
    assertTrue(mEvents.isEmpty());
  }

  private static void configureWindow(LynxView lynxView, View root, IBinder windowToken) {
    Resources resources = mock(Resources.class);
    DisplayMetrics displayMetrics = new DisplayMetrics();
    displayMetrics.density = DENSITY;
    when(resources.getDisplayMetrics()).thenReturn(displayMetrics);
    when(lynxView.getResources()).thenReturn(resources);
    when(lynxView.getWindowToken()).thenReturn(windowToken);
    when(lynxView.getRootView()).thenReturn(root);
  }

  private static CapturingRootView createWindowRoot(IBinder windowToken) {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    CapturingRootView root = new CapturingRootView(context, windowToken);
    root.layout(0, 0, ROOT_WIDTH, ROOT_HEIGHT);
    return root;
  }

  private static class CapturingRootView extends View {
    private final IBinder mWindowToken;
    private final List<CapturedEvent> mEvents = new ArrayList<>();
    private boolean mDispatchResult = true;

    CapturingRootView(Context context, IBinder windowToken) {
      super(context);
      mWindowToken = windowToken;
    }

    @Override
    public IBinder getWindowToken() {
      return mWindowToken;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
      mEvents.add(new CapturedEvent(event.getActionMasked(), event.getX(), event.getY(),
          event.getPointerId(0), event.getDownTime()));
      return mDispatchResult;
    }

    List<CapturedEvent> getCapturedEvents() {
      return mEvents;
    }

    void setDispatchResult(boolean dispatchResult) {
      mDispatchResult = dispatchResult;
    }
  }

  private static class CapturedEvent {
    final int action;
    final float x;
    final float y;
    final int pointerId;
    final long downTime;

    CapturedEvent(int action, float x, float y, int pointerId, long downTime) {
      this.action = action;
      this.x = x;
      this.y = y;
      this.pointerId = pointerId;
      this.downTime = downTime;
    }
  }
}
