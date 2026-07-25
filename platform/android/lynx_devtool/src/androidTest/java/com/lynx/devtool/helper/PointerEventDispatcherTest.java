// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.os.IBinder;
import android.view.MotionEvent;
import android.view.View;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.LynxView;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class PointerEventDispatcherTest {
  private LynxView mLynxView;
  private View mWindowRoot;
  private IBinder mWindowToken;
  private PointerEventDispatcher mDispatcher;

  @Before
  public void setUp() {
    mLynxView = mock(LynxView.class);
    mWindowRoot = mock(View.class);
    mWindowToken = mock(IBinder.class);
    configureWindow(mLynxView, mWindowRoot, mWindowToken, 100, 200, 10, 20);
    mDispatcher = new PointerEventDispatcher(new WeakReference<>(mLynxView));
  }

  @Test
  public void injectsDownAndUpThroughSessionWindow() {
    List<float[]> dispatchedEvents = new ArrayList<>();
    doAnswer(invocation -> {
      MotionEvent event = invocation.getArgument(0);
      dispatchedEvents.add(new float[] {event.getActionMasked(), event.getX(), event.getY()});
      return true;
    })
        .when(mWindowRoot)
        .dispatchTouchEvent(any(MotionEvent.class));

    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 12.f, 24.f, 0.f, 0.f, 7, 0, 1000));
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_UP, 12.f, 24.f, 0.f, 0.f, 7, 0, 2000));

    assertEquals(2, dispatchedEvents.size());
    assertEquals(MotionEvent.ACTION_DOWN, (int) dispatchedEvents.get(0)[0]);
    assertEquals(102.f, dispatchedEvents.get(0)[1], 0.f);
    assertEquals(204.f, dispatchedEvents.get(0)[2], 0.f);
    assertEquals(MotionEvent.ACTION_UP, (int) dispatchedEvents.get(1)[0]);
    verify(mWindowRoot, times(2)).dispatchTouchEvent(any(MotionEvent.class));
    verify(mLynxView, never()).dispatchTouchEvent(any(MotionEvent.class));
  }

  @Test
  public void rejectsMoveWithWrongPointerId() {
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 12.f, 24.f, 0.f, 0.f, 7, 0, 1000));
    assertFalse(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_MOVE, 20.f, 30.f, 0.f, 0.f, 8, 0, 2000));

    verify(mWindowRoot, times(1)).dispatchTouchEvent(any(MotionEvent.class));
  }

  @Test
  public void rejectsRepeatedDown() {
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 12.f, 24.f, 0.f, 0.f, 7, 0, 1000));
    assertFalse(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 20.f, 30.f, 0.f, 0.f, 8, 0, 2000));

    verify(mWindowRoot, times(1)).dispatchTouchEvent(any(MotionEvent.class));
  }

  @Test
  public void detachCancelsActiveSequence() {
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 12.f, 24.f, 0.f, 0.f, 7, 0, 1000));

    mDispatcher.detach();

    verify(mWindowRoot, times(2)).dispatchTouchEvent(any(MotionEvent.class));
  }

  @Test
  public void rejectsInputWhenSessionViewIsDetachedFromWindow() {
    when(mLynxView.getWindowToken()).thenReturn(null);

    assertFalse(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 12.f, 24.f, 0.f, 0.f, 7, 0, 1000));
    verify(mWindowRoot, never()).dispatchTouchEvent(any(MotionEvent.class));

    when(mLynxView.getWindowToken()).thenReturn(mWindowToken);
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 12.f, 24.f, 0.f, 0.f, 7, 0, 2000));
    verify(mWindowRoot, times(1)).dispatchTouchEvent(any(MotionEvent.class));
  }

  @Test
  public void rejectsRootFromDifferentWindow() {
    when(mWindowRoot.getWindowToken()).thenReturn(mock(IBinder.class));

    assertFalse(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 12.f, 24.f, 0.f, 0.f, 7, 0, 1000));
    verify(mWindowRoot, never()).dispatchTouchEvent(any(MotionEvent.class));
  }

  @Test
  public void attachSwitchesInputToNewSessionWindow() {
    LynxView secondLynxView = mock(LynxView.class);
    View secondWindowRoot = mock(View.class);
    IBinder secondWindowToken = mock(IBinder.class);
    configureWindow(secondLynxView, secondWindowRoot, secondWindowToken, 300, 400, 30, 40);

    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 12.f, 24.f, 0.f, 0.f, 7, 0, 1000));
    mDispatcher.attach(secondLynxView);
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_DOWN, 12.f, 24.f, 0.f, 0.f, 8, 0, 2000));
    assertTrue(mDispatcher.injectPointerEvent(
        PointerEventDispatcher.POINTER_EVENT_UP, 12.f, 24.f, 0.f, 0.f, 8, 0, 3000));

    verify(mWindowRoot, times(2)).dispatchTouchEvent(any(MotionEvent.class));
    verify(secondWindowRoot, times(2)).dispatchTouchEvent(any(MotionEvent.class));
  }

  private static void configureWindow(LynxView lynxView, View windowRoot, IBinder windowToken,
      int lynxX, int lynxY, int rootX, int rootY) {
    when(lynxView.getWindowToken()).thenReturn(windowToken);
    when(lynxView.getRootView()).thenReturn(windowRoot);
    when(windowRoot.getWindowToken()).thenReturn(windowToken);
    setLocationInWindow(lynxView, lynxX, lynxY);
    setLocationInWindow(windowRoot, rootX, rootY);
  }

  private static void setLocationInWindow(View view, int x, int y) {
    doAnswer(invocation -> {
      int[] location = invocation.getArgument(0);
      location[0] = x;
      location[1] = y;
      return null;
    })
        .when(view)
        .getLocationInWindow(any(int[].class));
  }
}
