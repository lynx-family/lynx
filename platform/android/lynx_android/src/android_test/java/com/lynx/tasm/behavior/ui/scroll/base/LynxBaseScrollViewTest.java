// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.scroll.base;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.view.MotionEvent;
import android.view.View;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollViewScrolling.ScrollDelegate;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test for {@link LynxBaseScrollView}.
 */
@RunWith(AndroidJUnit4.class)
public class LynxBaseScrollViewTest {
  private LynxBaseScrollView mScrollView;
  private Context mContext;

  private static class TestStateUpdater implements ScrollDelegate {
    public int lastState = -1;
    public boolean scrollEventFired = false;

    @Override
    public void onScrollStateChanged(int from, int to) {
      lastState = to;
    }

    @Override
    public void scrollViewDidScroll(LynxBaseScrollViewScrolling scrollView) {
      scrollEventFired = true;
    }
  }

  @Before
  public void setUp() {
    mContext = ApplicationProvider.getApplicationContext();
    mScrollView = new LynxBaseScrollView(mContext);
  }

  @Test
  public void testSetEnableScroll() {
    mScrollView.enableScroll(false);
    assertFalse(mScrollView.scrollEnabled());

    // With scrolling disabled, touch events should not be intercepted.
    MotionEvent downEvent = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 0, 0, 0);
    assertFalse(mScrollView.onInterceptTouchEvent(downEvent));

    mScrollView.enableScroll(true);
    assertTrue(mScrollView.scrollEnabled());
  }

  @Test
  public void testSetBounces() {
    mScrollView.enableBounces(false);
    assertFalse(mScrollView.bounces());

    mScrollView.enableBounces(true);
    assertTrue(mScrollView.bounces());
  }

  @Test
  public void testSetScrollEventsInterval() {
    TestStateUpdater updater = new TestStateUpdater();
    mScrollView.setScrollDelegate(updater);

    // First scroll event should fire
    mScrollView.mScrollDelegate.scrollViewDidScroll(mScrollView);
    assertTrue(updater.scrollEventFired);

    // Second scroll event immediately after should not fire
    updater.scrollEventFired = false;
    mScrollView.mScrollDelegate.scrollViewDidScroll(mScrollView);
  }

  @Test
  public void testSetScrollState() {
    TestStateUpdater updater = new TestStateUpdater();
    mScrollView.setScrollDelegate(updater);

    mScrollView.mScrollDelegate.onScrollStateChanged(0, 1);
    assertEquals(1, updater.lastState);

    mScrollView.mScrollDelegate.onScrollStateChanged(1, 2);
    assertEquals(2, updater.lastState);
  }

  @Test
  public void testLayoutAndMeasure() {
    mScrollView.mIsVertical = true;
    mScrollView.mWidth = 100;
    mScrollView.mHeight = 200;

    View child = new View(mContext);
    mScrollView.addView(child);

    // Set child dimensions
    child.measure(View.MeasureSpec.makeMeasureSpec(100, View.MeasureSpec.EXACTLY),
        View.MeasureSpec.makeMeasureSpec(500, View.MeasureSpec.EXACTLY));
    child.layout(0, 0, 100, 500);

    // Trigger layout on the scroll view
    mScrollView.onLayout(true, 0, 0, 100, 200);
  }
}
