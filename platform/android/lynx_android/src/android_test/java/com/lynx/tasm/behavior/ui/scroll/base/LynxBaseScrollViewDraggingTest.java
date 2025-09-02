// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.scroll.base;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.view.MotionEvent;
import android.view.ViewConfiguration;
import androidx.core.view.ViewCompat;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;

public class LynxBaseScrollViewDraggingTest {
  private LynxBaseScrollViewDragging mDragging;
  private LynxBaseScrollViewScroller mScroller;
  private LynxContext mLynxContext;

  @Before
  public void setUp() {
    mLynxContext = TestingUtils.getLynxContext();
    mScroller = mock(LynxBaseScrollViewScroller.class);

    ViewConfiguration viewConfiguration = mock(ViewConfiguration.class);
    when(viewConfiguration.getScaledWindowTouchSlop()).thenReturn(8);
    when(viewConfiguration.getScaledMinimumFlingVelocity()).thenReturn(50);
    when(viewConfiguration.getScaledMaximumFlingVelocity()).thenReturn(8000);

    mDragging = new LynxBaseScrollViewDragging(mLynxContext, null, 0);
    mDragging.mScrollHelper = mScroller;
  }

  @Test
  public void testIsBouncing() {
    mDragging.mIsVertical = true;
    mDragging.setScrollContentSizeVertically(400);
    mDragging.mHeight = 200;
    mDragging.scrollTo(0, 300);
    assertTrue(
        mDragging.isBouncingForwards(mDragging.getScrollY(), mDragging.getScrollRangeVertically()));
    mDragging.scrollTo(0, -100);
    assertTrue(mDragging.isBouncingBackwards(
        mDragging.getScrollY(), mDragging.getScrollRangeVertically()));
    mDragging.scrollTo(0, 100);
    assertFalse(
        mDragging.isBouncingForwards(mDragging.getScrollY(), mDragging.getScrollRangeVertically()));
    assertFalse(mDragging.isBouncingBackwards(
        mDragging.getScrollY(), mDragging.getScrollRangeVertically()));
  }

  @Test
  public void testGetFlingRange() {
    mDragging.mIsVertical = true;
    mDragging.setScrollContentSizeVertically(400); // height 200, range 200
    mDragging.mHeight = 200;
    mDragging.scrollTo(0, 100);
    mDragging.enableBounces(true);
    assertArrayEquals(new int[] {-300, 500}, mDragging.getFlingRange(true));
    mDragging.enableBounces(false);
    assertArrayEquals(new int[] {0, 200}, mDragging.getFlingRange(true));
  }

  @Test
  public void onTouchEvent() {
    mDragging.mIsVertical = true;
    mDragging.setScrollContentSizeVertically(400);

    MotionEvent downEvent = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 50, 50, 0);
    mDragging.onInterceptTouchEvent(downEvent);
    mDragging.onTouchEvent(downEvent);

    MotionEvent moveEvent = MotionEvent.obtain(0, 0, MotionEvent.ACTION_MOVE, 50, 100, 0);
    mDragging.onInterceptTouchEvent(moveEvent);
    mDragging.onTouchEvent(moveEvent);

    assertEquals(LynxBaseScrollViewScrolling.SCROLL_STATE_DRAGGING, mDragging.currentScrollState());
  }

  @Test
  public void testGetNestedScrollAxis() {
    mDragging.mIsVertical = true;
    assertEquals(ViewCompat.SCROLL_AXIS_VERTICAL, mDragging.getNestedScrollAxis());
    mDragging.mIsVertical = false;
    assertEquals(ViewCompat.SCROLL_AXIS_HORIZONTAL, mDragging.getNestedScrollAxis());
  }
}
