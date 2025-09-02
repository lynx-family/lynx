// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.scroll.base;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollViewScroller.ScrollFinishedCallback;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;

public class LynxBaseScrollViewScrollingTest {
  private LynxBaseScrollViewScrolling mScrollView;
  private LynxBaseScrollViewScroller mScroller;
  private ScrollFinishedCallback mCallback;
  private LynxBaseScrollViewScrolling.ScrollDelegate mScrollDelegate;

  @Before
  public void setUp() {
    mScroller = mock(LynxBaseScrollViewScroller.class);
    mCallback = mock(ScrollFinishedCallback.class);
    mScrollDelegate = mock(LynxBaseScrollViewScrolling.ScrollDelegate.class);
    LynxContext lynxContext = TestingUtils.getLynxContext();

    mScrollView = new LynxBaseScrollViewScrolling(lynxContext, null, 0);
    mScrollView.mScrollHelper = mScroller;
    mScrollView.mWidth = 100;
    mScrollView.mHeight = 200;
    mScrollView.setScrollDelegate(mScrollDelegate);
  }

  @Test
  public void testEnableScroll() {
    mScrollView.enableScroll(false);
    assertFalse(mScrollView.scrollEnabled());
    mScrollView.enableScroll(true);
    assertTrue(mScrollView.scrollEnabled());
  }

  @Test
  public void testStopScrolling() {
    mScrollView.stopScrolling();
    verify(mScroller, times(1)).stopAnimating(false);
    verify(mScroller, times(1)).stopAutoScroll();
    assertEquals(LynxBaseScrollViewScrolling.SCROLL_STATE_IDLE, mScrollView.currentScrollState());
  }

  @Test
  public void testGetScrollOffset() {
    mScrollView.scrollTo(10, 20);
    assertArrayEquals(new int[] {10, 20}, mScrollView.getScrollOffset());
  }

  @Test
  public void testSetScrollContentSize() {
    mScrollView.mIsVertical = true;
    mScrollView.setScrollContentSize(new int[] {300, 400});
    assertArrayEquals(new int[] {100, 400}, mScrollView.mContentSize);

    mScrollView.mIsVertical = false;
    mScrollView.setScrollContentSize(new int[] {500, 600});
    assertArrayEquals(new int[] {500, 200}, mScrollView.mContentSize);
  }

  @Test
  public void testScrollBy() {
    mScrollView.mIsVertical = true;
    mScrollView.setScrollContentSize(new int[] {100, 400}); // scroll range Y is 200
    mScrollView.scrollTo(0, 50);
    mScrollView.scrollBy(new int[] {0, 30});
    assertEquals(80, mScrollView.getScrollY());

    mScrollView.scrollBy(new int[] {0, 200}); // 80 + 200 = 280, clamped to 200
    assertEquals(200, mScrollView.getScrollY());

    mScrollView.mIsVertical = false;
    mScrollView.setScrollContentSize(new int[] {300, 200}); // scroll range X is 200
    mScrollView.scrollTo(50, 0);
    mScrollView.scrollBy(new int[] {40, 0});
    assertEquals(90, mScrollView.getScrollX());

    mScrollView.scrollBy(new int[] {200, 0}); // 90 + 200 = 290, clamped to 200
    assertEquals(200, mScrollView.getScrollX());
  }

  @Test
  public void testScrollTo() {
    mScrollView.mIsVertical = true;
    mScrollView.setScrollContentSize(new int[] {100, 400}); // scroll range Y is 200
    mScrollView.scrollTo(new int[] {0, 150});
    assertEquals(150, mScrollView.getScrollY());

    mScrollView.scrollTo(new int[] {0, 300}); // clamped to 200
    assertEquals(200, mScrollView.getScrollY());

    mScrollView.mIsVertical = false;
    mScrollView.setScrollContentSize(new int[] {300, 200}); // scroll range X is 200
    mScrollView.scrollTo(new int[] {80, 0});
    assertEquals(80, mScrollView.getScrollX());

    mScrollView.scrollTo(new int[] {250, 0}); // clamped to 200
    assertEquals(200, mScrollView.getScrollX());
  }

  @Test
  public void testAnimatedScrollTo() {
    mScrollView.mIsVertical = true;
    mScrollView.setScrollContentSize(new int[] {100, 400});
    mScrollView.animatedScrollTo(new int[] {0, 150}, mCallback);
    verify(mScroller, times(1)).animatedScrollTo(eq(0), eq(150), any(ScrollFinishedCallback.class));
    assertEquals(
        LynxBaseScrollViewScrolling.SCROLL_STATE_ANIMATING, mScrollView.currentScrollState());

    reset(mScroller);
    mScrollView.mIsVertical = false;
    mScrollView.setScrollContentSize(new int[] {300, 200});
    mScrollView.animatedScrollTo(new int[] {80, 0}, mCallback);
    verify(mScroller, times(1)).animatedScrollTo(eq(80), eq(0), any(ScrollFinishedCallback.class));
    assertEquals(
        LynxBaseScrollViewScrolling.SCROLL_STATE_ANIMATING, mScrollView.currentScrollState());
  }

  @Test
  public void testGetScrollRange() {
    mScrollView.mIsVertical = true;
    mScrollView.setScrollContentSize(new int[] {100, 400});
    assertArrayEquals(new int[] {0, 0, 0, 200}, mScrollView.getScrollRange());

    mScrollView.mIsVertical = false;
    mScrollView.setScrollContentSize(new int[] {300, 200});
    assertArrayEquals(new int[] {0, 200, 0, 0}, mScrollView.getScrollRange());
  }

  @Test
  public void testCanScrollForwards() {
    mScrollView.mIsVertical = true;
    mScrollView.setScrollContentSize(new int[] {100, 400});
    mScrollView.scrollTo(0, 100);
    assertTrue(mScrollView.canScrollForwards());
    mScrollView.scrollTo(0, 200);
    assertFalse(mScrollView.canScrollForwards());

    mScrollView.mIsVertical = false;
    mScrollView.setScrollContentSize(new int[] {300, 200});
    mScrollView.scrollTo(100, 0);
    assertTrue(mScrollView.canScrollForwards());
    mScrollView.scrollTo(200, 0);
    assertFalse(mScrollView.canScrollForwards());
  }

  @Test
  public void testCanScrollBackwards() {
    mScrollView.mIsVertical = true;
    mScrollView.setScrollContentSize(new int[] {100, 400});
    mScrollView.scrollTo(0, 100);
    assertTrue(mScrollView.canScrollBackwards());
    mScrollView.scrollTo(0, 0);
    assertFalse(mScrollView.canScrollBackwards());

    mScrollView.mIsVertical = false;
    mScrollView.setScrollContentSize(new int[] {300, 200});
    mScrollView.scrollTo(100, 0);
    assertTrue(mScrollView.canScrollBackwards());
    mScrollView.scrollTo(0, 0);
    assertFalse(mScrollView.canScrollBackwards());
  }

  @Test
  public void testTryToUpdateScrollState() {
    mScrollView.tryToUpdateScrollState(LynxBaseScrollViewScrolling.SCROLL_STATE_DRAGGING);
    verify(mScrollDelegate, times(1))
        .onScrollStateChanged(LynxBaseScrollViewScrolling.SCROLL_STATE_IDLE,
            LynxBaseScrollViewScrolling.SCROLL_STATE_DRAGGING);
    assertEquals(
        LynxBaseScrollViewScrolling.SCROLL_STATE_DRAGGING, mScrollView.currentScrollState());

    mScrollView.tryToUpdateScrollState(LynxBaseScrollViewScrolling.SCROLL_STATE_DRAGGING);
    verify(mScrollDelegate, times(1))
        .onScrollStateChanged(LynxBaseScrollViewScrolling.SCROLL_STATE_IDLE,
            LynxBaseScrollViewScrolling.SCROLL_STATE_DRAGGING);
  }

  @Test
  public void testUpdateProgrammaticallyScrollFinishedCallback() {
    ScrollFinishedCallback callback1 = mock(ScrollFinishedCallback.class);
    ScrollFinishedCallback callback2 = mock(ScrollFinishedCallback.class);

    mScrollView.updateProgrammaticallyScrollFinishedCallback(callback1);
    mScrollView.updateProgrammaticallyScrollFinishedCallback(callback2);
    verify(callback1, times(1)).finished(false);
    verify(callback2, never()).finished(false);

    reset(callback1, callback2);
    mScrollView.updateProgrammaticallyScrollFinishedCallback(null);
    verify(callback2, times(1)).finished(true);
  }

  @Test
  public void testScrollByUnlimited() {
    mScrollView.mIsVertical = true;
    mScrollView.setScrollContentSize(new int[] {100, 400}); // scroll range Y is 200
    mScrollView.scrollTo(0, 50);
    mScrollView.scrollByUnlimited(new int[] {0, 300}); // 50 + 300 = 350. No clamping.
    assertEquals(350, mScrollView.getScrollY());

    mScrollView.mIsVertical = false;
    mScrollView.setScrollContentSize(new int[] {300, 200}); // scroll range X is 200
    mScrollView.scrollTo(50, 0);
    mScrollView.scrollByUnlimited(new int[] {300, 0}); // 50 + 300 = 350. No clamping.
    assertEquals(350, mScrollView.getScrollX());
  }

  @Test
  public void testScrollToUnlimited() {
    mScrollView.mIsVertical = true;
    mScrollView.setScrollContentSize(new int[] {100, 400});
    mScrollView.scrollToUnlimited(new int[] {0, 350}); // No clamping
    assertEquals(350, mScrollView.getScrollY());

    mScrollView.mIsVertical = false;
    mScrollView.setScrollContentSize(new int[] {300, 200});
    mScrollView.scrollToUnlimited(new int[] {350, 0}); // No clamping
    assertEquals(350, mScrollView.getScrollX());
  }

  @Test
  public void testAnimatedScrollToUnlimited() {
    mScrollView.mIsVertical = true;
    mScrollView.setScrollContentSize(new int[] {100, 400});
    mScrollView.animatedScrollToUnlimited(new int[] {0, 350}, mCallback); // No clamping
    verify(mScroller, times(1)).animatedScrollTo(eq(0), eq(350), any(ScrollFinishedCallback.class));
    assertEquals(
        LynxBaseScrollViewScrolling.SCROLL_STATE_ANIMATING, mScrollView.currentScrollState());

    reset(mScroller);
    mScrollView.mIsVertical = false;
    mScrollView.setScrollContentSize(new int[] {300, 200});
    mScrollView.animatedScrollToUnlimited(new int[] {350, 0}, mCallback); // No clamping
    verify(mScroller, times(1)).animatedScrollTo(eq(350), eq(0), any(ScrollFinishedCallback.class));
    assertEquals(
        LynxBaseScrollViewScrolling.SCROLL_STATE_ANIMATING, mScrollView.currentScrollState());
  }

  @Test
  public void testEnableBounces() {
    mScrollView.enableBounces(false);
    assertFalse(mScrollView.bounces());
    mScrollView.enableBounces(true);
    assertTrue(mScrollView.bounces());
  }
}
