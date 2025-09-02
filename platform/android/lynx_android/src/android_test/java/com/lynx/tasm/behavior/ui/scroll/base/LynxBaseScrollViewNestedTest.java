// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.scroll.base;

import static androidx.core.view.ViewCompat.TYPE_TOUCH;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.widget.FrameLayout;
import androidx.core.view.ViewCompat;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test for {@link LynxBaseScrollViewNested}.
 * This test uses real view instances to verify nested scrolling behavior without mocking.
 */
@RunWith(AndroidJUnit4.class)
public class LynxBaseScrollViewNestedTest {
  private Context mContext;
  private LynxBaseScrollView mParentScrollView;
  private LynxBaseScrollView mChildScrollView;
  private FrameLayout mParentWrapper;

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();

    // Create a parent scroll view wrapped in a FrameLayout
    mParentWrapper = new FrameLayout(mContext);
    mParentScrollView = new LynxBaseScrollView(mContext);
    mParentScrollView.setScrollDelegate(new LynxBaseScrollViewScrolling.ScrollDelegate() {
      @Override
      public void onScrollStateChanged(int from, int to) {}

      @Override
      public void scrollViewDidScroll(LynxBaseScrollViewScrolling scrollView) {}
    });
    mParentWrapper.addView(mParentScrollView);

    // Create a child scroll view and add it to the parent
    mChildScrollView = new LynxBaseScrollView(mContext);
    mChildScrollView.setScrollDelegate(mParentScrollView.mScrollDelegate);
    mParentScrollView.addView(mChildScrollView);

    // Set content sizes to make them scrollable
    mParentScrollView.mIsVertical = true;
    mParentScrollView.setScrollContentSize(new int[] {100, 1000}); // Scroll range 900
    mParentScrollView.mHeight = 100;
    mChildScrollView.mIsVertical = true;
    mChildScrollView.setScrollContentSize(new int[] {100, 1000}); // Scroll range 900
    mParentScrollView.mHeight = 100;
  }

  @Test
  public void testOnStartNestedScroll() {
    // Parent should start nested scroll for a direct child
    mChildScrollView.startNestedScroll(ViewCompat.SCROLL_AXIS_VERTICAL, TYPE_TOUCH);
    boolean started = mChildScrollView.onStartNestedScroll(
        mParentScrollView, mParentScrollView, ViewCompat.SCROLL_AXIS_VERTICAL);
    assertTrue(started);
    mChildScrollView.onNestedScrollAccepted(
        mChildScrollView, mChildScrollView, ViewCompat.SCROLL_AXIS_VERTICAL, TYPE_TOUCH);
    assertEquals(ViewCompat.SCROLL_AXIS_VERTICAL, mChildScrollView.getNestedScrollAxes());
  }

  @Test
  public void testOnStopNestedScroll() {
    mParentScrollView.onStartNestedScroll(
        mChildScrollView, mChildScrollView, ViewCompat.SCROLL_AXIS_VERTICAL);
    mParentScrollView.onNestedScrollAccepted(
        mChildScrollView, mChildScrollView, ViewCompat.SCROLL_AXIS_VERTICAL, TYPE_TOUCH);

    assertEquals(ViewCompat.SCROLL_AXIS_VERTICAL, mParentScrollView.getNestedScrollAxes());

    mParentScrollView.onStopNestedScroll(mChildScrollView, TYPE_TOUCH);
    assertEquals(ViewCompat.SCROLL_AXIS_NONE, mParentScrollView.getNestedScrollAxes());
  }

  @Test
  public void testOnNestedPreScrollParentFirst() {
    mChildScrollView.setForwardNestedScrollMode(
        LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_PARENT_FIRST);
    mChildScrollView.setBackwardNestedScrollMode(
        LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_PARENT_FIRST);
    mParentScrollView.onStartNestedScroll(
        mChildScrollView, mChildScrollView, ViewCompat.SCROLL_AXIS_VERTICAL);

    int[] consumed = new int[2];
    // Scroll up by 100. Parent is at top, has room to scroll.
    mParentScrollView.onNestedPreScroll(mChildScrollView, 0, 100, consumed, TYPE_TOUCH);

    // Parent should consume the entire scroll because it has room.
    assertEquals(100, consumed[1]);
    assertEquals(100, mParentScrollView.getScrollY());
    assertEquals(0, mChildScrollView.getScrollY());
  }

  @Test
  public void testOnNestedPreScrollParentFirstParentAtLimit() {
    mChildScrollView.setForwardNestedScrollMode(
        LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_PARENT_FIRST);
    mChildScrollView.setBackwardNestedScrollMode(
        LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_PARENT_FIRST);

    mParentScrollView.onStartNestedScroll(
        mChildScrollView, mChildScrollView, ViewCompat.SCROLL_AXIS_VERTICAL);
    mChildScrollView.scrollTo(0, 900); // Scroll parent to its limit

    int[] consumed = new int[2];
    // Scroll up by 100. Parent is at limit, has no room.
    mParentScrollView.onNestedPreScroll(mChildScrollView, 0, 100, consumed, TYPE_TOUCH);

    // Parent should consume nothing.
    assertEquals(100, consumed[1]);
    assertEquals(100, mParentScrollView.getScrollY());
    assertEquals(900, mChildScrollView.getScrollY());
  }

  @Test
  public void testOnNestedScrollSelfFirst() {
    mChildScrollView.setForwardNestedScrollMode(
        LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_SELF_FIRST);
    mChildScrollView.setBackwardNestedScrollMode(
        LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_SELF_FIRST);
    mParentScrollView.onStartNestedScroll(
        mChildScrollView, mChildScrollView, ViewCompat.SCROLL_AXIS_VERTICAL);
    mChildScrollView.scrollTo(0, 900); // Scroll child to its limit

    // Child has scrolled, now report the unconsumed delta to the parent.
    // Child tried to scroll 50 more, but couldn't.
    mParentScrollView.onNestedScroll(mChildScrollView, 0, 900, 0, 50, TYPE_TOUCH);

    // Parent should consume the leftover 50.
    assertEquals(50, mParentScrollView.getScrollY());
    assertEquals(900, mChildScrollView.getScrollY());
  }

  @Test
  public void testOnNestedScrollParallel() {
    mChildScrollView.setForwardNestedScrollMode(
        LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_PARALLEL);
    mChildScrollView.setBackwardNestedScrollMode(
        LynxBaseScrollViewScroller.NESTED_SCROLL_MODE_PARALLEL);

    mParentScrollView.onStartNestedScroll(
        mChildScrollView, mChildScrollView, ViewCompat.SCROLL_AXIS_VERTICAL);

    int[] consumed = new int[2];
    // In parallel mode, onNestedPreScroll should do nothing.
    mParentScrollView.onNestedPreScroll(mChildScrollView, 0, 100, consumed, TYPE_TOUCH);
    assertArrayEquals(new int[] {0, 100}, consumed);

    // The scroll is then handled in onNestedScroll.
    // Let's say the child consumed 50 of the 100.
    mParentScrollView.onNestedScroll(mChildScrollView, 0, 50, 0, 50, TYPE_TOUCH);

    // The parent should consume the remaining 50.
    assertEquals(150, mParentScrollView.getScrollY());
  }
}
