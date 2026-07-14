// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list;

import static org.junit.Assert.assertEquals;

import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxSnapHelperTest {
  private static final int VIEWPORT_WIDTH = 300;
  private LynxContext lynxContext;

  @Before
  public void setUp() throws Exception {
    lynxContext = TestingUtils.getLynxContext();
  }

  @Test
  public void testSnap() {
    double snapAlignmentMillisecondsPerPx = 100f / lynxContext.getScreenMetrics().densityDpi;
    LynxSnapHelper snapHelper = createVerticalSnapHelper(
        500, 500, 10, 3, 20, 0, 20, snapAlignmentMillisecondsPerPx, 1, null);

    int[] out = snapHelper.findTargetSnapOffset(0, 100, true, false);
    assertEquals(0, out[0]);
    assertEquals(20, out[1]);
  }

  @Test
  public void testSnapMaxSnapCountKeepsSingleStepByDefault() {
    SnapResult snapResult = new SnapResult();
    LynxSnapHelper snapHelper = createVerticalSnapHelper(100, 100, 10, 10, 1, snapResult);

    // Verify that maxSnapCount = 1 keeps the original single-step snap behavior even for a
    // high-velocity fling.
    int[] out = snapHelper.findTargetSnapOffset(0, 10000, true, false);

    assertEquals(100, out[1]);
    assertEquals(1, snapResult.position);
    assertEquals(100, snapResult.targetOffsetY);
  }

  @Test
  public void testSnapMaxSnapCountLimitsMultiStepTarget() {
    SnapResult snapResult = new SnapResult();
    LynxSnapHelper snapHelper = createVerticalSnapHelper(100, 100, 10, 10, 3, snapResult);

    // Verify that maxSnapCount > 1 enables multi-step snapping while limiting the target to
    // the configured candidate range.
    int[] out = snapHelper.findTargetSnapOffset(0, 10000, true, false);

    assertEquals(300, out[1]);
    assertEquals(3, snapResult.position);
    assertEquals(300, snapResult.targetOffsetY);
  }

  @Test
  public void testSnapExtraDistanceStartsAboveVelocityThreshold() {
    SnapResult snapResult = new SnapResult();
    LynxSnapHelper snapHelper = createVerticalSnapHelper(100, 100, 10, 10, 3, snapResult);

    int[] out = snapHelper.findTargetSnapOffset(0, 2000, true, false);

    assertEquals(100, out[1]);
    assertEquals(1, snapResult.position);

    out = snapHelper.findTargetSnapOffset(0, 3000, true, false);

    assertEquals(200, out[1]);
    assertEquals(2, snapResult.position);
  }

  @Test
  public void testSnapMaxSnapCountCollapsesBoundaryCandidates() {
    SnapResult snapResult = new SnapResult();
    LynxSnapHelper snapHelper = createVerticalSnapHelper(250, 80, 5, 50, 3, snapResult);

    // Verify that multiple snap offsets clamped to the same boundary collapse into one
    // representative candidate, so the same boundary offset is not counted repeatedly.
    snapHelper.findTargetSnapOffset(0, 10000, true, false);

    assertEquals(2, snapResult.position);
  }

  private LynxSnapHelper createVerticalSnapHelper(int viewportHeight, int itemHeight, int itemCount,
      int currentOffset, int maxSnapCount, SnapResult snapResult) {
    return createVerticalSnapHelper(viewportHeight, itemHeight, itemCount, itemCount, currentOffset,
        0, 0, 0, maxSnapCount, snapResult);
  }

  private LynxSnapHelper createVerticalSnapHelper(int viewportHeight, int itemHeight, int itemCount,
      int childrenCount, int currentOffset, double snapAlignmentFactor, int snapAlignmentOffset,
      double snapAlignmentMillisecondsPerPx, int maxSnapCount, SnapResult snapResult) {
    ScrollView scrollView = new ScrollView(lynxContext);
    scrollView.setLayoutParams(new ViewGroup.LayoutParams(VIEWPORT_WIDTH, viewportHeight));
    scrollView.layout(0, 0, VIEWPORT_WIDTH, viewportHeight);

    LinearLayout linearLayout = new LinearLayout(lynxContext);
    linearLayout.setOrientation(LinearLayout.VERTICAL);
    int contentHeight = itemHeight * itemCount;
    scrollView.addView(linearLayout,
        new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT));
    linearLayout.layout(0, 0, VIEWPORT_WIDTH, contentHeight);

    for (int i = 0; i < itemCount; i++) {
      AndroidView view = new AndroidView(lynxContext);
      UIView ui = new UIView(lynxContext);
      view.bindDrawChildHook(ui);
      ui.setTop(itemHeight * i);
      ui.setHeight(itemHeight);
      view.setLayoutParams(new ViewGroup.LayoutParams(VIEWPORT_WIDTH, itemHeight));
      view.layout(0, itemHeight * i, VIEWPORT_WIDTH, itemHeight * (i + 1));
      linearLayout.addView(view);
    }
    scrollView.scrollTo(0, currentOffset);

    return new LynxSnapHelper(snapAlignmentFactor, snapAlignmentOffset,
        snapAlignmentMillisecondsPerPx, maxSnapCount, new LynxSnapHelper.LynxSnapHooks() {
          @Override
          public int getScrollX() {
            return scrollView.getScrollX();
          }

          @Override
          public int getScrollY() {
            return scrollView.getScrollY();
          }

          @Override
          public int getScrollHeight() {
            return viewportHeight;
          }

          @Override
          public int getScrollWidth() {
            return VIEWPORT_WIDTH;
          }

          @Override
          public int getContentHeight() {
            return contentHeight;
          }

          @Override
          public int getContentWidth() {
            return VIEWPORT_WIDTH;
          }

          @Override
          public int getChildrenCount() {
            return childrenCount;
          }

          @Override
          public int getVirtualChildrenCount() {
            return childrenCount;
          }

          @Override
          public View getChildAtIndex(int index) {
            return linearLayout.getChildAt(index);
          }

          @Override
          public View getViewAtPosition(int position) {
            return linearLayout.getChildAt(position);
          }

          @Override
          public int getIndexFromView(View view) {
            return linearLayout.indexOfChild(view);
          }

          @Override
          public void willSnapTo(int position, int currentOffsetX, int currentOffsetY,
              int targetOffsetX, int targetOffsetY) {
            if (snapResult != null) {
              snapResult.position = position;
              snapResult.targetOffsetY = targetOffsetY;
            }
          }
        });
  }

  private static class SnapResult {
    int position = -1;
    int targetOffsetY = 0;
  }
}
