// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.scroll;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.view.View;
import androidx.core.view.ViewCompat;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mockito;

public class BounceGestureHelperTest {
  private LynxContext mLynxContext;
  private AndroidScrollView mAndroidScrollView;
  private NestedHorizontalScrollView mHorizontalScrollView;
  private BounceGestureHelper mVerticalHelper;
  private BounceGestureHelper mHorizontalHelper;

  @Before
  public void setUp() throws Exception {
    mLynxContext = TestingUtils.getLynxContext();
    UIScrollView uiScrollView = mock(UIScrollView.class);
    // Init vertical ScrollView
    mAndroidScrollView = new AndroidScrollView(mLynxContext, uiScrollView);
    mVerticalHelper = mAndroidScrollView.mBounceGestureHelper;
    // Init horizontal ScrollView
    mHorizontalScrollView = mAndroidScrollView.getHScrollView();
    mHorizontalHelper = mHorizontalScrollView.mBounceGestureHelper;
  }

  @Test
  public void onStopNestedScroll() {
    // (1) Check for vertical
    mVerticalHelper.setBounceScrollRange(500);
    AndroidScrollView spyAndroidScrollView = spy(mAndroidScrollView);
    BounceGestureHelper spyVerticalHelper = spy(mVerticalHelper);
    registerBounceGestureHelper(spyVerticalHelper, spyAndroidScrollView);
    spyVerticalHelper.onStopNestedScroll(null, ViewCompat.TYPE_TOUCH);
    verify(spyAndroidScrollView)
        .smoothScrollToInternal(
            mAndroidScrollView.getScrollX(), mVerticalHelper.getBounceScrollRange());
    // (2) Check for TYPE_NON_TOUCH
    spyVerticalHelper.onStopNestedScroll(null, ViewCompat.TYPE_NON_TOUCH);
    verify(spyAndroidScrollView, Mockito.times(1))
        .smoothScrollToInternal(
            mAndroidScrollView.getScrollX(), mVerticalHelper.getBounceScrollRange());
    // (3) Check for horizontal
    mHorizontalHelper.setBounceScrollRange(500);
    NestedHorizontalScrollView spyHorizontalScrollView = spy(mHorizontalScrollView);
    BounceGestureHelper spyHorizontalHelper = spy(mHorizontalHelper);
    registerBounceGestureHelper(spyHorizontalHelper, spyHorizontalScrollView);
    spyHorizontalHelper.onStopNestedScroll(null, ViewCompat.TYPE_TOUCH);
    verify(spyHorizontalScrollView)
        .smoothScrollToInternal(
            mHorizontalHelper.getBounceScrollRange(), mHorizontalScrollView.getScrollY());
    // (4) Check for TYPE_NON_TOUCH
    spyHorizontalHelper.onStopNestedScroll(null, ViewCompat.TYPE_NON_TOUCH);
    verify(spyHorizontalScrollView, Mockito.times(1))
        .smoothScrollToInternal(
            mHorizontalHelper.getBounceScrollRange(), mHorizontalScrollView.getScrollY());
  }

  @Test
  public void onNestedPreScrollHorizontally() {
    NestedHorizontalScrollView spyBounceLayoutView = spy(mHorizontalScrollView);
    BounceGestureHelper spyHorizontalHelper = spy(mHorizontalHelper);
    registerBounceGestureHelper(spyHorizontalHelper, spyBounceLayoutView);
    View target = new View(mLynxContext);
    View spyTarget = spy(target);
    final int scrollRange = 1000;
    final int bounceScrollRange = 500;
    int delta = -20;
    int scrollX = 0;
    int scrollY = 0;
    int[] consumed = new int[2];
    // check for bounce start
    when(spyTarget.canScrollHorizontally(-1)).thenReturn(false);
    when(spyTarget.canScrollHorizontally(1)).thenReturn(true);
    // check for bounce start with delta < 0
    spyHorizontalHelper.onNestedPreScrollHorizontally(spyBounceLayoutView, spyTarget, scrollX,
        scrollY, delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollTo(0, scrollY);
    assertEquals(consumed[0], delta);
    scrollX = 100;
    spyHorizontalHelper.onNestedPreScrollHorizontally(spyBounceLayoutView, spyTarget, scrollX,
        scrollY, delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).requestDisallowInterceptTouchEvent(true);
    assertEquals(consumed[0], delta);
    // check for bounce start with delta > 0
    delta = 20;
    scrollX = 490;
    spyHorizontalHelper.onNestedPreScrollHorizontally(spyBounceLayoutView, spyTarget, scrollX,
        scrollY, delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollBy(bounceScrollRange - scrollX, 0);
    assertEquals(consumed[0], delta);
    delta = 5;
    spyHorizontalHelper.onNestedPreScrollHorizontally(spyBounceLayoutView, spyTarget, scrollX,
        scrollY, delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollBy(delta, 0);
    assertEquals(consumed[0], delta);

    // check for bounce end
    when(spyTarget.canScrollHorizontally(-1)).thenReturn(true);
    when(spyTarget.canScrollHorizontally(1)).thenReturn(false);
    // check for bounce end with delta > 0
    scrollX = 900;
    delta = 200;
    spyHorizontalHelper.onNestedPreScrollHorizontally(spyBounceLayoutView, spyTarget, scrollX,
        scrollY, delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollTo(scrollRange, scrollY);
    assertEquals(consumed[0], delta);
    delta = 100;
    spyHorizontalHelper.onNestedPreScrollHorizontally(spyBounceLayoutView, spyTarget, scrollX,
        scrollY, delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollBy(delta, 0);
    assertEquals(consumed[0], delta);
    // check for bounce end with delta < 0
    scrollX = 600;
    delta = -200;
    spyHorizontalHelper.onNestedPreScrollHorizontally(spyBounceLayoutView, spyTarget, scrollX,
        scrollY, delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollBy(bounceScrollRange - scrollX, 0);
    assertEquals(consumed[0], delta);
    delta = -50;
    spyHorizontalHelper.onNestedPreScrollHorizontally(spyBounceLayoutView, spyTarget, scrollX,
        scrollY, delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollBy(delta, 0);
    assertEquals(consumed[0], delta);
  }

  @Test
  public void onNestedPreScrollVertically() {
    NestedScrollView spyBounceLayoutView = spy(mAndroidScrollView);
    BounceGestureHelper spyVerticalHelper = spy(mVerticalHelper);
    registerBounceGestureHelper(spyVerticalHelper, spyBounceLayoutView);
    View target = new View(mLynxContext);
    View spyTarget = spy(target);
    final int scrollRange = 1000;
    final int bounceScrollRange = 500;
    int delta = -20;
    int scrollX = 0;
    int scrollY = 0;
    int[] consumed = new int[2];
    // check for bounce start
    when(spyTarget.canScrollVertically(-1)).thenReturn(false);
    when(spyTarget.canScrollVertically(1)).thenReturn(true);
    // check for bounce start with delta < 0
    spyVerticalHelper.onNestedPreScrollVertically(spyBounceLayoutView, spyTarget, scrollX, scrollY,
        delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollTo(scrollX, 0);
    assertEquals(consumed[1], delta);
    scrollY = 100;
    spyVerticalHelper.onNestedPreScrollVertically(spyBounceLayoutView, spyTarget, scrollX, scrollY,
        delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).requestDisallowInterceptTouchEvent(true);
    assertEquals(consumed[1], delta);
    // check for bounce start with delta > 0
    delta = 20;
    scrollY = 490;
    spyVerticalHelper.onNestedPreScrollVertically(spyBounceLayoutView, spyTarget, scrollX, scrollY,
        delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollBy(0, bounceScrollRange - scrollY);
    assertEquals(consumed[1], delta);
    delta = 5;
    spyVerticalHelper.onNestedPreScrollVertically(spyBounceLayoutView, spyTarget, scrollX, scrollY,
        delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollBy(0, delta);
    assertEquals(consumed[1], delta);

    // check for bounce end
    when(spyTarget.canScrollVertically(-1)).thenReturn(true);
    when(spyTarget.canScrollVertically(1)).thenReturn(false);
    // check for bounce end with delta > 0
    scrollY = 900;
    delta = 200;
    spyVerticalHelper.onNestedPreScrollVertically(spyBounceLayoutView, spyTarget, scrollX, scrollY,
        delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollTo(scrollX, scrollRange);
    assertEquals(consumed[1], delta);
    delta = 100;
    spyVerticalHelper.onNestedPreScrollVertically(spyBounceLayoutView, spyTarget, scrollX, scrollY,
        delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollBy(0, delta);
    assertEquals(consumed[1], delta);
    // check for bounce end with delta < 0
    scrollY = 600;
    delta = -200;
    spyVerticalHelper.onNestedPreScrollVertically(spyBounceLayoutView, spyTarget, scrollX, scrollY,
        delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollBy(0, bounceScrollRange - scrollY);
    assertEquals(consumed[1], delta);
    delta = -50;
    spyVerticalHelper.onNestedPreScrollVertically(spyBounceLayoutView, spyTarget, scrollX, scrollY,
        delta, scrollRange, bounceScrollRange, consumed);
    verify(spyBounceLayoutView).scrollBy(0, delta);
    assertEquals(consumed[1], delta);
  }

  @Test
  public void onNestedScrollHorizontally() {
    NestedHorizontalScrollView spyBounceLayoutView = spy(mHorizontalScrollView);
    BounceGestureHelper spyHorizontalHelper = spy(mHorizontalHelper);
    registerBounceGestureHelper(spyHorizontalHelper, spyBounceLayoutView);
    final int scrollRange = 1000;
    final int bounceScrollRange = 500;
    final int scrollX = 100;
    final int scrollY = 100;
    int deltaConsumed = -10;
    int deltaUnconsumed = -10;
    spyHorizontalHelper.onNestedScrollHorizontally(spyBounceLayoutView,
        spyBounceLayoutView.getHScroller(), scrollX, scrollY, deltaConsumed, deltaUnconsumed,
        scrollRange, bounceScrollRange);
    verify(spyBounceLayoutView).smoothScrollToInternal(0, scrollY);
    deltaConsumed = 10;
    deltaUnconsumed = 10;
    spyHorizontalHelper.onNestedScrollHorizontally(spyBounceLayoutView,
        spyBounceLayoutView.getHScroller(), scrollX, scrollY, deltaConsumed, deltaUnconsumed,
        scrollRange, bounceScrollRange);
    verify(spyBounceLayoutView).smoothScrollToInternal(scrollRange, scrollY);
  }

  @Test
  public void onNestedScrollVertically() {
    NestedScrollView spyBounceLayoutView = spy(mAndroidScrollView);
    BounceGestureHelper spyVerticalHelper = spy(mVerticalHelper);
    registerBounceGestureHelper(spyVerticalHelper, spyBounceLayoutView);
    final int scrollRange = 1000;
    final int bounceScrollRange = 500;
    final int scrollX = 100;
    final int scrollY = 100;
    int deltaConsumed = -10;
    int deltaUnconsumed = -10;
    spyVerticalHelper.onNestedScrollVertically(spyBounceLayoutView,
        spyBounceLayoutView.getVScroller(), scrollX, scrollY, deltaConsumed, deltaUnconsumed,
        scrollRange, bounceScrollRange);
    verify(spyBounceLayoutView).smoothScrollToInternal(scrollX, 0);
    deltaConsumed = 10;
    deltaUnconsumed = 10;
    spyVerticalHelper.onNestedScrollVertically(spyBounceLayoutView,
        spyBounceLayoutView.getVScroller(), scrollX, scrollY, deltaConsumed, deltaUnconsumed,
        scrollRange, bounceScrollRange);
    verify(spyBounceLayoutView).smoothScrollToInternal(scrollY, scrollRange);
  }

  private void registerBounceGestureHelper(
      BounceGestureHelper spyBounceGestureHelper, View spyView) {
    try {
      Field innerScrollView = BounceGestureHelper.class.getDeclaredField("mView");
      if (innerScrollView != null) {
        innerScrollView.setAccessible(true);
        innerScrollView.set(spyBounceGestureHelper, spyView);
      }
    } catch (NoSuchFieldException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }
}
