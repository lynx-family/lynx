// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.scroll.base;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.view.MotionEvent;
import androidx.annotation.Nullable;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.ui.scroll.base.LynxBaseScrollViewScroller.ScrollFinishedCallback;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Test for {@link LynxBaseScrollViewScroller}.
 * This test uses a real {@link LynxBaseScrollView} instance to test integration,
 * avoiding mocking libraries and access to private fields.
 */
@RunWith(AndroidJUnit4.class)
public class LynxBaseScrollViewScrollerTest {
  /**
   * A test-specific subclass of LynxBaseScrollView that acts as a "spy" to
   * record interactions from the scroller.
   */
  private static class TestLynxBaseScrollView extends LynxBaseScrollView {
    public boolean postOnAnimationCalled = false;
    public int totalDispatchedX = 0;
    public int totalDispatchedY = 0;

    public TestLynxBaseScrollView(Context context) {
      super(context);
    }

    @Override
    public void postOnAnimation(Runnable r) {
      // For tests, we just record that a new frame was requested.
      postOnAnimationCalled = true;
    }

    @Override
    public int[] dispatchScroll(int deltaX, int deltaY, int type, @Nullable MotionEvent event,
        @Nullable int[] offsetInWindow) {
      // This is a simplified implementation for testing.
      // It simulates the scroll and records the total distance dispatched.
      scrollBy(deltaX, deltaY);
      totalDispatchedX += deltaX;
      totalDispatchedY += deltaY;
      return new int[] {deltaX, deltaY};
    }
  }

  private static class FakeCallback implements ScrollFinishedCallback {
    public boolean finished = false;
    public boolean completed = false;

    @Override
    public void finished(boolean completed) {
      this.finished = true;
      this.completed = completed;
    }
  }

  private LynxBaseScrollViewScroller mScroller;
  private TestLynxBaseScrollView mScrollView;
  private FakeCallback mCallback;

  @Before
  public void setUp() {
    Context context = ApplicationProvider.getApplicationContext();
    mScroller = new LynxBaseScrollViewScroller(context);
    mScrollView = new TestLynxBaseScrollView(context);
    mScrollView.setScrollDelegate(new LynxBaseScrollViewScrolling.ScrollDelegate() {
      @Override
      public void onScrollStateChanged(int from, int to) {}

      @Override
      public void scrollViewDidScroll(LynxBaseScrollViewScrolling scrollView) {}
    });
    mCallback = new FakeCallback();
    mScroller.scrollView = mScrollView;
  }

  @Test
  public void testAnimatedScrollInitiatesScroll() {
    mScrollView.scrollTo(0, 0);
    mScroller.animatedScrollTo(100, 200, mCallback);

    // Verify the scroller has started by checking if it requested a new animation frame.
    assertTrue(mScrollView.postOnAnimationCalled);
    // Verify it hasn't finished yet.
    assertFalse(mCallback.finished);

    // Simulate the first animation frame
    mScroller.run();

    // Verify that a scroll command was dispatched to the view
    assertEquals(mScrollView.getScrollY(), mScrollView.totalDispatchedY);
  }

  @Test
  public void testFlingInitiatesFling() {
    mScrollView.scrollTo(0, 0);
    mScrollView.mIsVertical = true;
    mScrollView.mHeight = 200;
    mScrollView.setScrollContentSizeVertically(400); // Scroll range is 200

    mScroller.fling(1000, 2000, mCallback);

    // Verify the scroller has started
    assertTrue(mScrollView.postOnAnimationCalled);
    assertFalse(mCallback.finished);

    // Simulate the first animation frame
    mScroller.run();

    // Verify that a scroll command was dispatched to the view
    assertEquals(mScrollView.getScrollY(), mScrollView.totalDispatchedY);
  }

  @Test
  public void testTryBouncesBackInitiatesScroll() {
    mScrollView.mHeight = 200;
    mScrollView.setScrollContentSizeVertically(400); // Scroll range is 200

    // Bouncing backwards
    mScrollView.scrollTo(0, -50);
    assertTrue(mScroller.tryBouncesBack(mScrollView, true, mCallback));
    assertTrue(mScrollView.postOnAnimationCalled);
    assertFalse(mCallback.finished);
  }

  @Test
  public void testStopAnimating() {
    mScroller.animatedScrollTo(100, 200, mCallback);

    // Before stopping, it should not be finished
    assertFalse(mCallback.finished);

    mScroller.stopAnimating(true);
    assertTrue(mCallback.finished);
    assertTrue(mCallback.completed);

    // Reset for next assertion
    mCallback.finished = false;
    mScroller.animatedScrollTo(100, 200, mCallback);

    assertFalse(mCallback.finished);
    mScroller.stopAnimating(false);
    assertTrue(mCallback.finished);
    assertFalse(mCallback.completed);
  }

  @Test
  public void testRunFinishesWhenScrollIsComplete() {
    // Start a scroll to the current location (delta is 0)
    mScrollView.scrollTo(100, 200);
    mScroller.animatedScrollTo(100, 200, mCallback);

    // The scroller will start, but its internal OverScroller has no distance to travel.
    assertTrue(mScrollView.postOnAnimationCalled);
    assertFalse(mCallback.finished);

    // Reset the flag to check if run() requests another frame. It shouldn't.
    mScrollView.postOnAnimationCalled = false;

    // The first run should detect that the animation is already finished.
    mScroller.run();
    mScroller.stopAnimating(true);

    // Verify the scroller stopped and called the callback with 'completed = true'
    assertTrue(mCallback.finished);
    assertTrue(mCallback.completed);

    // Verify that it did NOT request another animation frame, because it's finished.
    assertTrue(mScrollView.postOnAnimationCalled);
  }
}
