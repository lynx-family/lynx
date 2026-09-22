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
    assertTrue(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 300.f, 400.f, 0.f, 0.f, 7, 0, 1000));
    assertTrue(injectSinglePointerEvent(
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
    assertFalse(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, -1.f, 20.f, 0.f, 0.f, 7, 0, 1000));
    assertFalse(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 400.f, 20.f, 0.f, 0.f, 7, 0, 1000));
    assertFalse(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 20.f, 600.f, 0.f, 0.f, 7, 0, 1000));

    assertTrue(mEvents.isEmpty());
  }

  @Test
  public void injectsMultiPointerSequence() {
    assertTrue(mDispatcher.injectPointerEvent(PointerEventDispatcher.POINTER_EVENT_DOWN,
        new int[] {7}, new float[] {10.f}, new float[] {20.f}, 7, 0, 1000));
    assertTrue(mDispatcher.injectPointerEvent(PointerEventDispatcher.POINTER_EVENT_DOWN,
        new int[] {7, 8}, new float[] {10.f, 30.f}, new float[] {20.f, 40.f}, 8, 0, 2000));
    assertTrue(mDispatcher.injectPointerEvent(PointerEventDispatcher.POINTER_EVENT_MOVE,
        new int[] {7, 8}, new float[] {12.f, 32.f}, new float[] {22.f, 42.f}, 7, 0, 3000));
    assertTrue(mDispatcher.injectPointerEvent(PointerEventDispatcher.POINTER_EVENT_UP,
        new int[] {7, 8}, new float[] {12.f, 32.f}, new float[] {22.f, 42.f}, 8, 0, 4000));
    assertTrue(mDispatcher.injectPointerEvent(PointerEventDispatcher.POINTER_EVENT_UP,
        new int[] {7}, new float[] {12.f}, new float[] {22.f}, 7, 0, 5000));

    assertEquals(5, mEvents.size());
    assertEquals(MotionEvent.ACTION_DOWN, mEvents.get(0).action);
    assertEquals(MotionEvent.ACTION_POINTER_DOWN, mEvents.get(1).action);
    assertEquals(1, mEvents.get(1).actionIndex);
    assertEquals(2, mEvents.get(1).pointerIds.length);
    assertEquals(7, mEvents.get(1).pointerIds[0]);
    assertEquals(8, mEvents.get(1).pointerIds[1]);
    assertEquals(MotionEvent.ACTION_MOVE, mEvents.get(2).action);
    assertEquals(24.f, mEvents.get(2).xs[0], 0.f);
    assertEquals(64.f, mEvents.get(2).xs[1], 0.f);
    assertEquals(MotionEvent.ACTION_POINTER_UP, mEvents.get(3).action);
    assertEquals(1, mEvents.get(3).actionIndex);
    assertEquals(MotionEvent.ACTION_UP, mEvents.get(4).action);
    assertEquals(mEvents.get(0).downTime, mEvents.get(4).downTime);
  }

  @Test
  public void rejectsRepeatedDownAndPreservesActivePointer() {
    assertTrue(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 1000));
    assertFalse(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 20.f, 30.f, 0.f, 0.f, 8, 0, 2000));
    assertTrue(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_UP, 10.f, 20.f, 0.f, 0.f, 7, 0, 3000));

    assertEquals(2, mEvents.size());
    assertEquals(MotionEvent.ACTION_DOWN, mEvents.get(0).action);
    assertEquals(MotionEvent.ACTION_UP, mEvents.get(1).action);
  }

  @Test
  public void rejectsWrongPointerIdAndAcceptsOriginalPointer() {
    assertTrue(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 1000));
    assertFalse(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_UP, 20.f, 30.f, 0.f, 0.f, 8, 0, 2000));
    assertTrue(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_UP, 10.f, 20.f, 0.f, 0.f, 7, 0, 4000));

    assertEquals(2, mEvents.size());
    assertEquals(7, mEvents.get(0).pointerId);
    assertEquals(7, mEvents.get(1).pointerId);
  }

  @Test
  public void detachCancelsActiveSequence() {
    assertTrue(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 1000));

    mDispatcher.detach();

    assertEquals(2, mEvents.size());
    assertEquals(MotionEvent.ACTION_CANCEL, mEvents.get(1).action);
    assertEquals(7, mEvents.get(1).pointerId);
    assertFalse(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 8, 0, 2000));
  }

  @Test
  public void attachCancelsOldWindowAndUsesCurrentWindow() {
    assertTrue(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 1000));

    LynxView nextLynxView = mock(LynxView.class);
    IBinder nextWindowToken = mock(IBinder.class);
    CapturingRootView nextWindowRoot = createWindowRoot(nextWindowToken);
    configureWindow(nextLynxView, nextWindowRoot, nextWindowToken);
    List<CapturedEvent> nextEvents = nextWindowRoot.getCapturedEvents();
    mDispatcher.attach(nextLynxView);

    assertTrue(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 30.f, 40.f, 0.f, 0.f, 8, 0, 2000));
    assertTrue(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_UP, 30.f, 40.f, 0.f, 0.f, 8, 0, 3000));

    assertEquals(2, mEvents.size());
    assertEquals(MotionEvent.ACTION_CANCEL, mEvents.get(1).action);
    assertEquals(2, nextEvents.size());
    assertEquals(8, nextEvents.get(0).pointerId);
  }

  @Test
  public void returnsWindowDispatchResult() {
    mWindowRoot.setDispatchResult(false);

    assertFalse(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 1000));

    mWindowRoot.setDispatchResult(true);
    assertTrue(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 2000));
  }

  @Test
  public void rejectsDetachedLynxView() {
    when(mLynxView.getWindowToken()).thenReturn(null);

    assertFalse(injectSinglePointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 10.f, 20.f, 0.f, 0.f, 7, 0, 1000));
    assertTrue(mEvents.isEmpty());
  }

  private boolean injectSinglePointerEvent(int type, float x, float y, float deltaX, float deltaY,
      int pointerId, int modifiers, long timestampUs) {
    return mDispatcher.injectPointerEvent(type, new int[] {pointerId}, new float[] {x},
        new float[] {y}, pointerId, modifiers, timestampUs);
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
      int pointerCount = event.getPointerCount();
      int[] pointerIds = new int[pointerCount];
      float[] xs = new float[pointerCount];
      float[] ys = new float[pointerCount];
      for (int index = 0; index < pointerCount; ++index) {
        pointerIds[index] = event.getPointerId(index);
        xs[index] = event.getX(index);
        ys[index] = event.getY(index);
      }
      mEvents.add(new CapturedEvent(event.getActionMasked(), event.getActionIndex(), pointerIds, xs,
          ys, event.getDownTime()));
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
    final int actionIndex;
    final int[] pointerIds;
    final float[] xs;
    final float[] ys;
    final float x;
    final float y;
    final int pointerId;
    final long downTime;

    CapturedEvent(
        int action, int actionIndex, int[] pointerIds, float[] xs, float[] ys, long downTime) {
      this.action = action;
      this.actionIndex = actionIndex;
      this.pointerIds = pointerIds;
      this.xs = xs;
      this.ys = ys;
      this.x = xs[0];
      this.y = ys[0];
      this.pointerId = pointerIds[0];
      this.downTime = downTime;
    }
  }
}
