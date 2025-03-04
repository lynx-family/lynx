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
    int getChildrenCount();
    int getVirtualChildrenCount();
    View getChildAtIndex(int index);
    View getViewAtPosition(int position);
    int getIndexFromView(View view);
    void willSnapTo(
        int position, int currentOffsetX, int currentOffsetY, int targetOffsetX, int targetOffsetY);
  }

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

  private int distanceToItem(@NonNull View targetView) {
    if (targetView instanceof AndroidView
        && ((AndroidView) targetView).getDrawChildHook() instanceof LynxBaseUI) {
      LynxBaseUI ui = (LynxBaseUI) ((AndroidView) targetView).getDrawChildHook();
      if (mIsVertical) {
        final int childPosition =
            (int) (ui.getTop() + (ui.getHeight() * mSnapAlignmentFactor) + mSnapAlignmentOffset);
        final int containerPosition = (int) (mSnapHooks.getScrollY()
            + mSnapHooks.getScrollHeight() * mSnapAlignmentFactor + mSnapAlignmentOffset);
        return childPosition - containerPosition;
      } else {
        final int childPosition =
            (int) (ui.getLeft() + (ui.getWidth() * mSnapAlignmentFactor) + mSnapAlignmentOffset);
        final int containerPosition = (int) (mSnapHooks.getScrollX()
            + mSnapHooks.getScrollWidth() * mSnapAlignmentFactor + mSnapAlignmentOffset);
        return childPosition - containerPosition;
      }
    } else {
      throw new RuntimeException("A list-item is not an AndroidView, some thing went wrong");
    }
  }

  public int[] findTargetSnapOffset(
      int velocityX, int velocityY, boolean isVertical, boolean isRtl) {
    mIsVertical = isVertical;
    mIsRtl = isRtl;
    int[] out = new int[2];

    int position = findTargetSnapPosition(velocityX, velocityY);
    int offset = 0;
    if (position != -1) {
      // check if position is illegal
      position = Math.min(Math.max(position, 0), mSnapHooks.getVirtualChildrenCount() - 1);

      if (position != -1) {
        View view = mSnapHooks.getViewAtPosition(position);
        if (view != null) {
          if (view instanceof AndroidView
              && ((AndroidView) view).getDrawChildHook() instanceof LynxBaseUI) {
            LynxBaseUI ui = (LynxBaseUI) ((AndroidView) view).getDrawChildHook();
            if (mIsVertical) {
              offset = (int) (ui.getTop()
                  - (mSnapHooks.getScrollHeight() - ui.getHeight()) * mSnapAlignmentFactor
                  + mSnapAlignmentOffset);
            } else {
              offset = (int) (ui.getLeft()
                  - (mSnapHooks.getScrollWidth() - ui.getWidth()) * mSnapAlignmentFactor
                  + mSnapAlignmentOffset);
            }
          } else {
            throw new RuntimeException(
                "The target list-item is not an AndroidView, some thing went wrong");
          }
        }
      }
    }

    if (mIsVertical) {
      out[0] = 0;
      out[1] = offset;
    } else {
      out[0] = offset;
      out[1] = 0;
    }

    mSnapHooks.willSnapTo(
        position, mSnapHooks.getScrollX(), mSnapHooks.getScrollY(), out[0], out[1]);

    return out;
  }

  private int findTargetSnapPosition(int velocityX, int velocityY) {
    boolean hasVelocity = mIsVertical ? velocityY != 0 : velocityX != 0;

    // A child that is exactly in the position is eligible for both before and after
    View closestChildBeforePosition = null;
    int distanceBefore = Integer.MIN_VALUE;
    View closestChildAfterPosition = null;
    int distanceAfter = Integer.MAX_VALUE;

    // Find the first view before the position, and the first view after the position
    final int childCount = mSnapHooks.getChildrenCount();
    for (int i = 0; i < childCount; i++) {
      final View child = mSnapHooks.getChildAtIndex(i);
      if (child == null) {
        continue;
      }
      final int distance = distanceToItem(child);

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

    int targetPosition = -1;

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

    if (targetPosition != -1) {
      return targetPosition;
    }

    // There is no child in the direction of the fling. Either it doesn't exist (start/end of
    // the list), or it is not yet attached (very rare case when children are larger then the
    // viewport). Extrapolate from the child that is visible to get the position of the view to
    // snap to.
    View visibleView = forwardDirection ? closestChildBeforePosition : closestChildAfterPosition;
    if (visibleView == null) {
      return -1;
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
