// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list;

import android.view.View;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.view.AndroidView;

public class LynxSnapHelper {
  public interface LynxSnapHooks {
    int getScrollX();
    int getScrollY();
    int getScrollHeight();
    int getScrollWidth();
    int getContentHeight();
    int getContentWidth();
    int getChildrenCount();
    int getVirtualChildrenCount();
    View getChildAtIndex(int index);
    View getViewAtPosition(int position);
    int getIndexFromView(View view);
    void willSnapTo(
        int position, int currentOffsetX, int currentOffsetY, int targetOffsetX, int targetOffsetY);
  }
  private static final int INVALID_INDEX = -1;
  private LynxSnapHooks mSnapHooks = null;
  private boolean mIsVertical = true;
  private boolean mIsRtl = false;
  private double mSnapAlignmentFactor = -1;
  private int mSnapAlignmentOffset = 0;
  public double mSnapAlignmentMillisecondsPerPx = 0;

  public LynxSnapHelper(double snapAlignmentFactor, int snapAlignmentOffset,
      double snapAlignmentMillisecondsPerPx, LynxSnapHooks snapHooks) {
    mSnapAlignmentFactor = snapAlignmentFactor;
    mSnapAlignmentOffset = snapAlignmentOffset;
    mSnapAlignmentMillisecondsPerPx = snapAlignmentMillisecondsPerPx;
    mSnapHooks = snapHooks;
  }

  private int getListItemSnapScrollOffset(@NonNull View targetView) {
    if (targetView instanceof AndroidView
        && ((AndroidView) targetView).getDrawChildHook() instanceof LynxBaseUI) {
      LynxBaseUI ui = (LynxBaseUI) ((AndroidView) targetView).getDrawChildHook();
      if (mIsVertical) {
        return (int) (ui.getTop()
            - (mSnapHooks.getScrollHeight() - ui.getHeight()) * mSnapAlignmentFactor
            + mSnapAlignmentOffset);
      } else {
        return (int) (ui.getLeft()
            - (mSnapHooks.getScrollWidth() - ui.getWidth()) * mSnapAlignmentFactor
            + mSnapAlignmentOffset);
      }
    } else {
      throw new RuntimeException("A list-item is not an AndroidView, some thing went wrong");
    }
  }

  public int[] findTargetSnapOffset(
      int velocityX, int velocityY, boolean isVertical, boolean isRtl) {
    mIsVertical = isVertical;
    mIsRtl = isRtl;
    int[] out = new int[] {0, 0};

    int position = findTargetSnapPosition(velocityX, velocityY);
    int offset = mIsVertical ? mSnapHooks.getScrollY() : mSnapHooks.getScrollX();
    if (position != INVALID_INDEX) {
      // check if position is illegal
      position = Math.min(Math.max(position, 0), mSnapHooks.getVirtualChildrenCount() - 1);
      if (position != INVALID_INDEX) {
        View view = mSnapHooks.getViewAtPosition(position);
        if (view != null) {
          offset = getListItemSnapScrollOffset(view);
        } else {
          position = INVALID_INDEX;
        }
      }
    }

    if (mIsVertical) {
      out[1] = offset;
    } else {
      out[0] = offset;
    }
    mSnapHooks.willSnapTo(
        position, mSnapHooks.getScrollX(), mSnapHooks.getScrollY(), out[0], out[1]);
    return out;
  }

  private int findTargetSnapPosition(int velocityX, int velocityY) {
    boolean hasVelocity = mIsVertical ? velocityY != 0 : velocityX != 0;

    // A child that is exactly in the position is eligible for both before and after
    View closestChildBeforePosition = null;
    View closestChildAfterPosition = null;
    View clampedChildBeforePosition = null;
    View clampedChildAfterPosition = null;
    int distanceBefore = Integer.MIN_VALUE;
    int distanceAfter = Integer.MAX_VALUE;
    int contentOffset = mIsVertical ? mSnapHooks.getScrollY() : mSnapHooks.getScrollX();
    int minScrollRange = 0;
    int maxScrollRange = Math.max(0,
        mIsVertical ? mSnapHooks.getContentHeight() - mSnapHooks.getScrollHeight()
                    : mSnapHooks.getContentWidth() - mSnapHooks.getScrollWidth());
    // Find the first view before the position, and the first view after the position
    final int childCount = mSnapHooks.getChildrenCount();
    for (int i = 0; i < childCount; i++) {
      View child = mSnapHooks.getChildAtIndex(i);
      if (child == null) {
        continue;
      }
      int itemSnapOffset = getListItemSnapScrollOffset(child);
      int clampedItemSnapOffset = itemSnapOffset;
      if (clampedItemSnapOffset > maxScrollRange) {
        clampedItemSnapOffset = maxScrollRange;
        // Consider child's itemSnapOffset may be clamped by maxScrollRange, here we choose
        // the child which has the min itemSnapOffset.
        if (clampedChildAfterPosition == null
            || itemSnapOffset < getListItemSnapScrollOffset(clampedChildAfterPosition)) {
          clampedChildAfterPosition = child;
        } else {
          // Use clampedChildAfterPosition instead of current list item.
          child = clampedChildAfterPosition;
        }
      } else if (clampedItemSnapOffset < minScrollRange) {
        // Consider child's itemSnapOffset may be clamped by minScrollRange, here we choose
        // the child which has the max itemSnapOffset.
        clampedItemSnapOffset = minScrollRange;
        if (clampedChildBeforePosition == null
            || itemSnapOffset > getListItemSnapScrollOffset(clampedChildBeforePosition)) {
          clampedChildBeforePosition = child;
        } else {
          // Use clampedChildBeforePosition instead of current list item.
          child = clampedChildBeforePosition;
        }
      }
      final int distance = clampedItemSnapOffset - contentOffset;
      if (distance <= 0 && distance > distanceBefore) {
        // Choose child with clampedItemSnapOffset is nearest-before
        // content offset.
        distanceBefore = distance;
        closestChildBeforePosition = child;
      }
      if (distance >= 0 && distance < distanceAfter) {
        // Choose child with clampedItemSnapOffset is nearest-after content
        // offset.
        distanceAfter = distance;
        closestChildAfterPosition = child;
      }
    }

    int targetPosition = INVALID_INDEX;

    final boolean forwardDirection = isForwardFling(velocityX, velocityY);

    if (!hasVelocity) {
      if (closestChildAfterPosition != null && closestChildBeforePosition != null) {
        if (Math.abs(distanceAfter) < Math.abs(distanceBefore)) {
          targetPosition = mSnapHooks.getIndexFromView(closestChildAfterPosition);
        } else {
          targetPosition = mSnapHooks.getIndexFromView(closestChildBeforePosition);
        }
      } else if (closestChildAfterPosition != null) {
        targetPosition = mSnapHooks.getIndexFromView(closestChildAfterPosition);
      } else if (closestChildBeforePosition != null) {
        targetPosition = mSnapHooks.getIndexFromView(closestChildBeforePosition);
      }
    } else {
      // Return the position of the first child from the position, in the direction of the fling
      if (forwardDirection && closestChildAfterPosition != null) {
        targetPosition = mSnapHooks.getIndexFromView(closestChildAfterPosition);
      } else if (!forwardDirection && closestChildBeforePosition != null) {
        targetPosition = mSnapHooks.getIndexFromView(closestChildBeforePosition);
      }
    }

    if (targetPosition != INVALID_INDEX) {
      return targetPosition;
    }

    // There is no child in the direction of the fling. Either it doesn't exist (start/end of
    // the list), or it is not yet attached (very rare case when children are larger then the
    // viewport). Extrapolate from the child that is visible to get the position of the view to
    // snap to.
    View visibleView = forwardDirection ? closestChildBeforePosition : closestChildAfterPosition;
    if (visibleView == null) {
      return INVALID_INDEX;
    }
    int visiblePosition = mSnapHooks.getIndexFromView(visibleView);

    boolean forwardDirectionWithRTL =
        mIsVertical ? forwardDirection : (mIsRtl ? !forwardDirection : forwardDirection);

    return visiblePosition + (!forwardDirectionWithRTL ? -1 : +1);
  }

  private boolean isForwardFling(int velocityX, int velocityY) {
    if (mIsVertical) {
      return velocityY >= 0;
    } else {
      return velocityX >= 0;
    }
  }
}
