// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.scroll;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import androidx.annotation.Nullable;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.ui.scroll.ScrollSnapHelper.ScrollContainerHooks;
import com.lynx.tasm.behavior.ui.scroll.ScrollSnapHelper.SnapItem;
import com.lynx.tasm.behavior.ui.scroll.ScrollSnapHelper.SnapTarget;
import com.lynx.tasm.utils.PixelUtils;
import com.lynx.testing.base.TestingUtils;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class ScrollSnapHelperTest {
  @Before
  public void setUp() {
    TestingUtils.getLynxContext();
  }

  @Test
  public void testReadableMapConstructorAndAlignment() {
    // Verify that the ReadableMap constructor parses factor, offset, and maxSnapCount and aligns
    // the item with the viewport end.
    JavaOnlyMap params = new JavaOnlyMap();
    params.putDouble("factor", 1);
    params.putInt("offset", 10);
    params.putInt("maxSnapCount", 2);
    TestHooks hooks = new TestHooks(
        100, 0, 300, 200, Arrays.asList(new SnapItem(0, 0, 100), new SnapItem(1, 200, 100)));

    SnapTarget target = new ScrollSnapHelper(params, hooks).findSnapTarget(0);

    assertTrue(target.hasTarget());
    assertEquals(1, target.index);
    assertEquals(100 + (int) PixelUtils.dipToPx(10), target.targetOffset);
  }

  @Test
  public void testNearestTargetPrefersBeforeWhenDistancesTie() {
    // Verify that zero velocity selects the item nearest to the current offset and prefers the
    // preceding item when the distances are equal.
    TestHooks hooks = new TestHooks(
        50, 0, 200, 100, Arrays.asList(new SnapItem(0, 0, 100), new SnapItem(1, 100, 100)));
    ScrollSnapHelper helper = new ScrollSnapHelper(0, 0, 1, hooks);

    SnapTarget target = helper.findSnapTarget(0);

    assertTrue(target.hasTarget());
    assertEquals(0, target.index);
    assertEquals(0, target.targetOffset);
  }

  @Test
  public void testVelocitySelectsFirstTargetInFlingDirection() {
    // Verify that single-item snapping selects the first candidate after or before the current
    // offset according to the velocity direction.
    TestHooks hooks = new TestHooks(50, 0, 200, 100,
        Arrays.asList(
            new SnapItem(0, 0, 100), new SnapItem(1, 100, 100), new SnapItem(2, 200, 100)));
    ScrollSnapHelper helper = new ScrollSnapHelper(0, 0, 1, hooks);

    SnapTarget forwardTarget = helper.findSnapTarget(1000);
    SnapTarget backwardTarget = helper.findSnapTarget(-1000);

    assertEquals(1, forwardTarget.index);
    assertEquals(100, forwardTarget.targetOffset);
    assertEquals(0, backwardTarget.index);
    assertEquals(0, backwardTarget.targetOffset);
  }

  @Test
  public void testAdjacentItemFallback() {
    // Verify that a virtualized container can use the hook to return an adjacent item when no
    // attached candidate exists in the fling direction.
    TestHooks hooks =
        new TestHooks(150, 0, 300, 100, Collections.singletonList(new SnapItem(1, 100, 100)));
    hooks.mAdjacentItem = new SnapItem(2, 200, 100);
    ScrollSnapHelper helper = new ScrollSnapHelper(0, 0, 1, hooks);

    SnapTarget target = helper.findSnapTarget(1000);

    assertEquals(2, target.index);
    assertEquals(200, target.targetOffset);
    assertEquals(1, hooks.mAdjacentSourceIndex);
    assertTrue(hooks.mAdjacentForward);
  }

  @Test
  public void testMissingAdjacentItemReturnsNoTarget() {
    // Verify that the result keeps the current offset and has no target when neither an attached
    // item nor an adjacent item from the hook exists in the fling direction.
    TestHooks hooks =
        new TestHooks(150, 0, 300, 100, Collections.singletonList(new SnapItem(1, 100, 100)));
    ScrollSnapHelper helper = new ScrollSnapHelper(0, 0, 1, hooks);

    SnapTarget target = helper.findSnapTarget(1000);

    assertFalse(target.hasTarget());
    assertEquals(-1, target.index);
    assertEquals(150, target.targetOffset);
    assertEquals(-1, hooks.mSnapTargetIndex);
    assertEquals(150, hooks.mSnapCurrentOffset);
    assertEquals(150, hooks.mSnapTargetOffset);
  }

  @Test
  public void testMultiStepTargetRespectsMaxSnapCount() {
    // Verify that a high-velocity fling can cross multiple items without exceeding maxSnapCount
    // in either direction.
    List<SnapItem> items = createItems(6, 100);
    TestHooks hooks = new TestHooks(50, 0, 500, 100, items);
    ScrollSnapHelper helper = new ScrollSnapHelper(0, 0, 3, hooks);

    SnapTarget forwardTarget = helper.findSnapTarget(10000);
    hooks.mCurrentOffset = 450;
    SnapTarget backwardTarget = helper.findSnapTarget(-10000);

    assertEquals(3, forwardTarget.index);
    assertEquals(300, forwardTarget.targetOffset);
    assertEquals(2, backwardTarget.index);
    assertEquals(200, backwardTarget.targetOffset);
  }

  @Test
  public void testExtraDistanceStartsAboveVelocityThreshold() {
    TestHooks hooks = new TestHooks(10, 0, 900, 100, createItems(10, 100));
    ScrollSnapHelper helper = new ScrollSnapHelper(0, 0, 3, hooks);

    SnapTarget thresholdTarget = helper.findSnapTarget(2000);
    SnapTarget projectedTarget = helper.findSnapTarget(3000);

    assertEquals(1, thresholdTarget.index);
    assertEquals(100, thresholdTarget.targetOffset);
    assertEquals(2, projectedTarget.index);
    assertEquals(200, projectedTarget.targetOffset);
  }

  @Test
  public void testMultiStepSnapCollapsesBoundaryCandidates() {
    TestHooks hooks = new TestHooks(50, 0, 150, 250,
        Arrays.asList(new SnapItem(0, 0, 80), new SnapItem(1, 80, 80), new SnapItem(2, 160, 80),
            new SnapItem(3, 240, 80), new SnapItem(4, 320, 80)));

    SnapTarget target = new ScrollSnapHelper(0, 0, 3, hooks).findSnapTarget(10000);

    // Items 2-4 share the end boundary; the item whose raw offset first reaches it represents it.
    assertEquals(2, target.index);
    assertEquals(150, target.targetOffset);
  }

  @Test
  public void testMultiStepSnapCountsRawIndicesWhenEventIndicesHaveGaps() {
    // Verify that source indices can include a skipped bounce child, which counts toward
    // maxSnapCount even though the bounce view is not a snap candidate.
    TestHooks hooks = new TestHooks(50, 0, 300, 100,
        Arrays.asList(new SnapItem(0, 0, 100), new SnapItem(1, 100, 100), new SnapItem(3, 200, 100),
            new SnapItem(4, 300, 100)));
    ScrollSnapHelper helper = new ScrollSnapHelper(0, 0, 3, hooks);
    SnapTarget target = helper.findSnapTarget(10000);
    assertEquals(3, target.index);
    assertEquals(200, target.targetOffset);
  }

  @Test
  public void testInvalidConfigurationFallsBackToSingleStep() {
    // Verify that invalid factor and maxSnapCount values report separate errors and fall back to
    // start alignment and single-item snapping.
    TestHooks hooks = new TestHooks(50, 0, 200, 100,
        Arrays.asList(
            new SnapItem(0, 0, 100), new SnapItem(1, 100, 100), new SnapItem(2, 200, 100)));
    ScrollSnapHelper helper = new ScrollSnapHelper(-1, 0, 0, hooks);

    SnapTarget target = helper.findSnapTarget(10000);

    assertEquals(2, hooks.mErrors.size());
    assertEquals(1, target.index);
    assertEquals(100, target.targetOffset);
  }

  @Test
  public void testInvalidContextAndItemsReturnNoTarget() {
    // Verify that an invalid viewport, no items, a null item, and an item with invalid negative
    // geometry do not produce a snap target.
    TestHooks invalidViewportHooks =
        new TestHooks(40, 0, 200, 0, Collections.singletonList(new SnapItem(0, 0, 100)));
    SnapTarget invalidViewportTarget =
        new ScrollSnapHelper(0, 0, 1, invalidViewportHooks).findSnapTarget(0);

    TestHooks invalidItemsHooks =
        new TestHooks(60, 0, 200, 100, Arrays.asList(null, new SnapItem(1, 100, -1)));
    SnapTarget invalidItemsTarget =
        new ScrollSnapHelper(0, 0, 1, invalidItemsHooks).findSnapTarget(0);

    assertFalse(invalidViewportTarget.hasTarget());
    assertEquals(-1, invalidViewportTarget.index);
    assertEquals(40, invalidViewportTarget.targetOffset);
    assertFalse(invalidItemsTarget.hasTarget());
    assertEquals(60, invalidItemsTarget.targetOffset);
  }

  @Test
  public void testZeroSizedItemRemainsSnapCandidate() {
    // Verify that a zero-sized child still provides a snap point for content overflowing from its
    // descendants.
    TestHooks hooks =
        new TestHooks(60, 0, 200, 100, Collections.singletonList(new SnapItem(1, 100, 0)));

    SnapTarget target = new ScrollSnapHelper(0, 0, 1, hooks).findSnapTarget(0);

    assertTrue(target.hasTarget());
    assertEquals(1, target.index);
    assertEquals(100, target.targetOffset);
  }

  @Test
  public void testOffsetsAreClampedToScrollRange() {
    // Verify that when multiple raw item offsets cross a scroll boundary, the first item reaching
    // that boundary is retained as the target.
    TestHooks hooks = new TestHooks(190, 0, 200, 100,
        Arrays.asList(new SnapItem(0, 0, 100), new SnapItem(1, 200, 100), new SnapItem(2, 300, 100),
            new SnapItem(3, 400, 100)));
    ScrollSnapHelper helper = new ScrollSnapHelper(0, 0, 3, hooks);

    SnapTarget target = helper.findSnapTarget(10000);

    assertTrue(target.hasTarget());
    assertEquals(1, target.index);
    assertEquals(200, target.targetOffset);
  }

  private static List<SnapItem> createItems(int count, int itemSize) {
    List<SnapItem> items = new ArrayList<>();
    for (int index = 0; index < count; index++) {
      items.add(new SnapItem(index, index * itemSize, itemSize));
    }
    return items;
  }

  private static final class TestHooks implements ScrollContainerHooks {
    private int mCurrentOffset;
    private final int mMinOffset;
    private final int mMaxOffset;
    private final int mViewportSize;
    private final List<SnapItem> mItems;
    private final List<String> mErrors = new ArrayList<>();
    @Nullable private SnapItem mAdjacentItem;
    private int mAdjacentSourceIndex = -1;
    private boolean mAdjacentForward;
    private int mSnapTargetIndex = Integer.MIN_VALUE;
    private int mSnapCurrentOffset;
    private int mSnapTargetOffset;

    TestHooks(
        int currentOffset, int minOffset, int maxOffset, int viewportSize, List<SnapItem> items) {
      mCurrentOffset = currentOffset;
      mMinOffset = minOffset;
      mMaxOffset = maxOffset;
      mViewportSize = viewportSize;
      mItems = items;
    }

    @Override
    public int getCurrentOffset() {
      return mCurrentOffset;
    }

    @Override
    public int getMinOffset() {
      return mMinOffset;
    }

    @Override
    public int getMaxOffset() {
      return mMaxOffset;
    }

    @Override
    public int getViewportSize() {
      return mViewportSize;
    }

    @Override
    public List<SnapItem> getSnapItems() {
      return mItems;
    }

    @Nullable
    @Override
    public SnapItem getAdjacentSnapItem(int index, boolean forward) {
      mAdjacentSourceIndex = index;
      mAdjacentForward = forward;
      return mAdjacentItem;
    }

    @Override
    public void willSnapTo(int index, int currentOffset, int targetOffset) {
      mSnapTargetIndex = index;
      mSnapCurrentOffset = currentOffset;
      mSnapTargetOffset = targetOffset;
    }

    @Override
    public void handleError(String message) {
      mErrors.add(message);
    }
  }
}
