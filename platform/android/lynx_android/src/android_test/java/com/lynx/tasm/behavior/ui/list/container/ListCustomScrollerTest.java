// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list.container;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.view.animation.Interpolator;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
@RunWith(AndroidJUnit4.class)
public class ListCustomScrollerTest {
  private ListCustomScroller mScroller;

  @Before
  public void setUp() throws Exception {
    LynxContext lynxContext = TestingUtils.getLynxContext();
    mScroller = new ListCustomScroller(lynxContext, new Interpolator() {
      @Override
      public float getInterpolation(float t) {
        t -= 1.0f;
        return t * t * t * t * t + 1.0f;
      }
    });
  }

  @Test
  public void testFriction() {
    mScroller.setFriction(0.5f);
  }

  @Test
  public void testIsFinished() {
    assertTrue(mScroller.isFinished());
  }

  @Test
  public void testGetCurrX() {
    assertEquals(mScroller.getCurrX(), 0);
  }

  @Test
  public void testGetCurrY() {
    assertEquals(mScroller.getCurrY(), 0);
  }

  @Test
  public void testGetStartX() {
    assertEquals(mScroller.getStartX(), 0);
  }

  @Test
  public void testGetStartY() {
    assertEquals(mScroller.getStartY(), 0);
  }

  @Test
  public void testGetDuration() {
    assertEquals(mScroller.getDuration(), 0);
  }

  @Test
  public void testComputeScrollOffset() {
    assertFalse(mScroller.computeScrollOffset());
    mScroller.startScroll(0, 0, 100, 100);
    mScroller.computeScrollOffset();
    mScroller.fling(0, 0, 100, 100, 0, 100, 0, 100);
    mScroller.computeScrollOffset();
    mScroller.fling(200, 200, 100, 100, 0, 100, 0, 100);
    mScroller.computeScrollOffset();
  }

  @Test
  public void testStartScroll() {
    mScroller.startScroll(0, 0, 0, 0);
  }

  @Test
  public void testFling() {
    mScroller.fling(0, 0, 0, 0, 0, 0, 0, 0);
  }

  @Test
  public void testAbortAnimation() {
    mScroller.abortAnimation();
  }

  @Test
  public void testIsScrollingInDirection() {
    assertEquals(mScroller.isScrollingInDirection(0, 0), false);
  }
}
