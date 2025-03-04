// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list;

import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.OrientationHelper;
import androidx.recyclerview.widget.PagerSnapHelper;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.SnapHelper;
import androidx.viewpager.widget.ViewPager;
import com.lynx.tasm.behavior.ui.list.container.NestedScrollContainerView;

/**
 * Implementation of the {@link SnapHelper} supporting pager style snapping in either vertical or
 * horizontal orientation.
 *
 * <p>
 *
 * FactoredPagerSnapHelper can help achieve a similar behavior to {@link ViewPager}.
 * Set both {@link RecyclerView} and the items of the
 * {@link RecyclerView.Adapter} to have
 * {@link android.view.ViewGroup.LayoutParams#MATCH_PARENT} height and width and then attach
 * PagerSnapHelper to the {@link RecyclerView} using {@link #attachToRecyclerView(RecyclerView)}.
 */
public class FactoredPagerSnapHelper extends PagerSnapHelper {
  public interface FactoredPagerHooks {
    void willSnapTo(
        int position, int currentOffsetX, int currentOffsetY, int distanceX, int distanceY);
  }

  public FactoredPagerHooks mPagerHooks = null;

  public RecyclerView mRecyclerView = null;

  private double mFactor = 0.5F;

  private int mOffset = 0;

  public void setPagerAlignFactor(double factor) {
    mFactor = factor;
  }

  public void setPagerAlignOffset(int offset) {
    mOffset = offset;
  }

  // Orientation helpers are lazily created per LayoutManager.
  @Nullable private OrientationHelper mVerticalHelper;
  @Nullable private OrientationHelper mHorizontalHelper;

  @Nullable
  @Override
  public int[] calculateDistanceToFinalSnap(
      @NonNull RecyclerView.LayoutManager layoutManager, @NonNull View targetView) {
    int[] out = new int[2];
    if (layoutManager.canScrollHorizontally()) {
      out[0] = distanceToItem(layoutManager, targetView, getHorizontalHelper(layoutManager));
    } else {
      out[0] = 0;
    }

    if (layoutManager.canScrollVertically()) {
      out[1] = distanceToItem(layoutManager, targetView, getVerticalHelper(layoutManager));
    } else {
      out[1] = 0;
    }

    if (mRecyclerView != null) {
      mPagerHooks.willSnapTo(layoutManager.getPosition(targetView), mRecyclerView.getScrollX(),
          mRecyclerView.getScrollY(), out[0], out[1]);
    }
    return out;
  }

  @Nullable
  @Override
  public View findSnapView(RecyclerView.LayoutManager layoutManager) {
    if (layoutManager.canScrollVertically()) {
      return findTargetView(layoutManager, getVerticalHelper(layoutManager));
    } else if (layoutManager.canScrollHorizontally()) {
      return findTargetView(layoutManager, getHorizontalHelper(layoutManager));
    }
    return null;
  }

  private int distanceToItem(@NonNull RecyclerView.LayoutManager layoutManager,
      @NonNull View targetView, OrientationHelper helper) {
    final int childPosition = (int) (helper.getDecoratedStart(targetView)
        + (helper.getDecoratedMeasurement(targetView) * mFactor) + mOffset);
    final int containerPosition;
    if (layoutManager.getClipToPadding()) {
      containerPosition =
          (int) (helper.getStartAfterPadding() + helper.getTotalSpace() * mFactor + mOffset);
    } else {
      containerPosition = (int) (helper.getEnd() * mFactor + mOffset);
    }
    return childPosition - containerPosition;
  }

  @Override
  public int findTargetSnapPosition(
      RecyclerView.LayoutManager layoutManager, int velocityX, int velocityY) {
    if (mRecyclerView == null) {
      return RecyclerView.NO_POSITION;
    }
    final int itemCount = layoutManager.getItemCount();
    if (itemCount == 0) {
      mPagerHooks.willSnapTo(-1, mRecyclerView.getScrollX(), mRecyclerView.getScrollY(),
          mRecyclerView.getScrollX(), mRecyclerView.getScrollY());
      return RecyclerView.NO_POSITION;
    }

    final OrientationHelper orientationHelper = layoutManager.canScrollVertically()
        ? getVerticalHelper(layoutManager)
        : getHorizontalHelper(layoutManager);

    // A child that is exactly in the position is eligible for both before and after
    View closestChildBeforePosition = null;
    int distanceBefore = Integer.MIN_VALUE;
    View closestChildAfterPosition = null;
    int distanceAfter = Integer.MAX_VALUE;

    // Find the first view before the position, and the first view after the position
    final int childCount = layoutManager.getChildCount();
    for (int i = 0; i < childCount; i++) {
      final View child = layoutManager.getChildAt(i);
      if (child == null) {
        continue;
      }
      final int distance = distanceToItem(layoutManager, child, orientationHelper);

      if (distance <= 0 && distance > distanceBefore) {
        // Child is before the position and closer then the previous best
        distanceBefore = distance;
        closestChildBeforePosition = child;
      }
      if (distance >= 0 && distance < distanceAfter) {
        // Child is after the position and closer then the previous best
        distanceAfter = distance;
        closestChildAfterPosition = child;
      }
    }

    // Return the position of the first child from the position, in the direction of the fling
    final boolean forwardDirection = isForwardFling(layoutManager, velocityX, velocityY);
    if (forwardDirection && closestChildAfterPosition != null) {
      return layoutManager.getPosition(closestChildAfterPosition);
    } else if (!forwardDirection && closestChildBeforePosition != null) {
      return layoutManager.getPosition(closestChildBeforePosition);
    }

    // There is no child in the direction of the fling. Either it doesn't exist (start/end of
    // the list), or it is not yet attached (very rare case when children are larger then the
    // viewport). Extrapolate from the child that is visible to get the position of the view to
    // snap to.
    View visibleView = forwardDirection ? closestChildBeforePosition : closestChildAfterPosition;
    if (visibleView == null) {
      return RecyclerView.NO_POSITION;
    }
    int visiblePosition = layoutManager.getPosition(visibleView);
    int snapToPosition = visiblePosition + (!forwardDirection ? -1 : +1);

    if (snapToPosition < 0) {
      snapToPosition = 0;
    }

    if (snapToPosition >= itemCount) {
      return RecyclerView.NO_POSITION;
    }
    return snapToPosition;
  }

  private boolean isForwardFling(
      RecyclerView.LayoutManager layoutManager, int velocityX, int velocityY) {
    if (layoutManager.canScrollHorizontally()) {
      return velocityX >= 0;
    } else {
      return velocityY >= 0;
    }
  }

  /**
   * Return the child view that is currently closest to the position of this parent.
   *
   * @param layoutManager The {@link RecyclerView.LayoutManager} associated with the attached
   *                      {@link RecyclerView}.
   * @param helper The relevant {@link OrientationHelper} for the attached {@link RecyclerView}.
   *
   * @return the child view that is currently closest to the position of this parent.
   */
  @Nullable
  private View findTargetView(RecyclerView.LayoutManager layoutManager, OrientationHelper helper) {
    if (mRecyclerView == null) {
      return null;
    }
    int childCount = layoutManager.getChildCount();
    if (childCount == 0) {
      mPagerHooks.willSnapTo(-1, mRecyclerView.getScrollX(), mRecyclerView.getScrollY(),
          mRecyclerView.getScrollX(), mRecyclerView.getScrollY());
      return null;
    }

    View closestChild = null;
    final int position;
    if (layoutManager.getClipToPadding()) {
      position = (int) (helper.getStartAfterPadding() + helper.getTotalSpace() * mFactor + mOffset);
    } else {
      position = (int) (helper.getEnd() * mFactor + mOffset);
    }
    int absClosest = Integer.MAX_VALUE;

    for (int i = 0; i < childCount; i++) {
      final View child = layoutManager.getChildAt(i);
      int childPosition = (int) (helper.getDecoratedStart(child)
          + (helper.getDecoratedMeasurement(child) * mFactor) + mOffset);
      int absDistance = Math.abs(childPosition - position);

      /** if child position is closer than previous closest, set it as closest  **/
      if (absDistance < absClosest) {
        absClosest = absDistance;
        closestChild = child;
      }
    }
    if (closestChild == null) {
      mPagerHooks.willSnapTo(-1, mRecyclerView.getScrollX(), mRecyclerView.getScrollY(),
          mRecyclerView.getScrollX(), mRecyclerView.getScrollY());
    }
    return closestChild;
  }

  @NonNull
  private OrientationHelper getVerticalHelper(@NonNull RecyclerView.LayoutManager layoutManager) {
    if (mVerticalHelper == null || mVerticalHelper.getLayoutManager() != layoutManager) {
      mVerticalHelper = OrientationHelper.createVerticalHelper(layoutManager);
    }
    return mVerticalHelper;
  }

  @NonNull
  private OrientationHelper getHorizontalHelper(@NonNull RecyclerView.LayoutManager layoutManager) {
    if (mHorizontalHelper == null || mHorizontalHelper.getLayoutManager() != layoutManager) {
      mHorizontalHelper = OrientationHelper.createHorizontalHelper(layoutManager);
    }
    return mHorizontalHelper;
  }

  @Override
  public void attachToRecyclerView(@Nullable RecyclerView recyclerView)
      throws IllegalStateException {
    super.attachToRecyclerView(recyclerView);
    mRecyclerView = recyclerView;
  }
}
