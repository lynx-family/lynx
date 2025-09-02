// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.scroll.base;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import androidx.core.view.ViewCompat;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;

public class LynxNestedScrollingChildHelperTest {
  private LynxBaseScrollView mView;
  private LynxNestedScrollingChildHelper mHelper;
  private LynxBaseScrollView mParent;

  @Before
  public void setUp() {
    LynxContext lynxContext = TestingUtils.getLynxContext();
    LynxBaseScrollViewScrolling.ScrollDelegate mScrollDelegate =
        new LynxBaseScrollViewScrolling.ScrollDelegate() {
          @Override
          public void onScrollStateChanged(int from, int to) {}

          @Override
          public void scrollViewDidScroll(LynxBaseScrollViewScrolling scrollView) {}
        };

    mView = new LynxBaseScrollView(lynxContext, null, 0);
    mView.mIsVertical = true;
    mView.setScrollDelegate(mScrollDelegate);
    mParent = new LynxBaseScrollView(lynxContext, null, 0);
    mParent.mIsVertical = true;
    mParent.addView(mView);
    mParent.setScrollDelegate(mScrollDelegate);
    mHelper = new LynxNestedScrollingChildHelper(mView);
  }

  @Test
  public void testSetNestedScrollingEnabled() {
    mHelper.setNestedScrollingEnabled(true);
    assertTrue(mHelper.isNestedScrollingEnabled());
    mHelper.setNestedScrollingEnabled(false);
    assertFalse(mHelper.isNestedScrollingEnabled());
  }

  @Test
  public void testStartNestedScroll() {
    mHelper.setNestedScrollingEnabled(true);
    assertTrue(mParent.onStartNestedScroll(mView, mView, ViewCompat.SCROLL_AXIS_VERTICAL));
    assertTrue(mHelper.startNestedScroll(ViewCompat.SCROLL_AXIS_VERTICAL));
    assertTrue(mHelper.hasNestedScrollingParent());
    mHelper.stopNestedScroll();
    assertFalse(mParent.onStartNestedScroll(mView, mView, ViewCompat.SCROLL_AXIS_HORIZONTAL));
    assertFalse(mHelper.startNestedScroll(ViewCompat.SCROLL_AXIS_HORIZONTAL));
  }

  @Test
  public void testStopNestedScroll() {
    mHelper.setNestedScrollingEnabled(true);
    mHelper.startNestedScroll(ViewCompat.SCROLL_AXIS_VERTICAL);
    mHelper.stopNestedScroll();
    ViewCompat.stopNestedScroll(mView);
  }

  @Test
  public void testDispatchNestedPreScroll() {
    mHelper.setNestedScrollingEnabled(true);
    mHelper.startNestedScroll(ViewCompat.SCROLL_AXIS_VERTICAL);

    int[] consumed = new int[2];
    int[] offsetInWindow = new int[2];
    mParent.onNestedPreScroll(mView, 10, 20, consumed);
    assertTrue(mHelper.dispatchNestedPreScroll(10, 20, consumed, offsetInWindow));
  }

  @Test
  public void testDispatchNestedScroll() {
    mHelper.setNestedScrollingEnabled(true);
    mHelper.startNestedScroll(ViewCompat.SCROLL_AXIS_VERTICAL);

    mParent.onNestedScroll(mView, 5, 5, 10, 10);
    assertTrue(mHelper.dispatchNestedScroll(5, 5, 10, 10, null));
  }

  @Test
  public void testDispatchNestedPreFling() {
    mView.mNestedScrollingChildHelper.setNestedScrollingEnabled(true);
    mView.mNestedScrollingChildHelper.startNestedScroll(ViewCompat.SCROLL_AXIS_VERTICAL);

    assertFalse(mParent.onNestedPreFling(mView, 100, 200));
    assertFalse(mView.mNestedScrollingChildHelper.dispatchNestedPreFling(100, 200));
  }

  @Test
  public void testDispatchNestedFling() {
    mView.mNestedScrollingChildHelper.setNestedScrollingEnabled(true);
    mView.mNestedScrollingChildHelper.startNestedScroll(ViewCompat.SCROLL_AXIS_VERTICAL);

    assertFalse(mParent.onNestedFling(mView, 100, 200, true));
    assertFalse(mView.mNestedScrollingChildHelper.dispatchNestedFling(100, 200, true));
  }
}
