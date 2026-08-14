// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list;

import android.view.View;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

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
  private static final int DEFAULT_MAX_SNAP_COUNT = 1;
  private static final double FLING_DISTANCE_GAIN = 1.0d;
  private static final double EXTRA_DISTANCE_VELOCITY_THRESHOLD = 2000.0d;
  private static final double VELOCITY_TO_VIEWPORT_RATIO = 2.0d;
  private LynxSnapHooks mSnapHooks = null;
  private boolean mIsVertical = true;
  private boolean mIsRtl = false;
  private float mSnapAlignmentFactor = -1;
  private int mSnapAlignmentOffset = 0;
  private int mMaxSnapCount = DEFAULT_MAX_SNAP_COUNT;

  public LynxSnapHelper(
      float snapAlignmentFactor, int snapAlignmentOffset, LynxSnapHooks snapHooks) {
    this(snapAlignmentFactor, snapAlignmentOffset, DEFAULT_MAX_SNAP_COUNT, snapHooks);
  }

  public LynxSnapHelper(float snapAlignmentFactor, int snapAlignmentOffset, int maxSnapCount,
      LynxSnapHooks snapHooks) {
    mSnapAlignmentFactor = snapAlignmentFactor;
    mSnapAlignmentOffset = snapAlignmentOffset;
    mMaxSnapCount = Math.max(DEFAULT_MAX_SNAP_COUNT, maxSnapCount);
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

    int position = INVALID_INDEX;
    boolean useMultiStepSnap = shouldUseMultiStepSnap(velocityX, velocityY);
    if (useMultiStepSnap) {
      position = findMultiStepTargetSnapPosition(velocityX, velocityY);
    }
    if (position == INVALID_INDEX) {
      position = findSingleStepTargetSnapPosition(velocityX, velocityY);
    }

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

  private boolean shouldUseMultiStepSnap(int velocityX, int velocityY) {
    return hasVelocity(velocityX, velocityY) && mMaxSnapCount > DEFAULT_MAX_SNAP_COUNT;
  }

  private boolean hasVelocity(int velocityX, int velocityY) {
    return mIsVertical ? velocityY != 0 : velocityX != 0;
  }

  private int findSingleStepTargetSnapPosition(int velocityX, int velocityY) {
    // A child that is exactly in the position is eligible for both before and after
    View closestChildBeforePosition = null;
    View closestChildAfterPosition = null;
    View clampedChildBeforePosition = null;
    View clampedChildAfterPosition = null;
    int distanceBefore = Integer.MIN_VALUE;
    int distanceAfter = Integer.MAX_VALUE;
    int contentOffset = mIsVertical ? mSnapHooks.getScrollY() : mSnapHooks.getScrollX();
    int minScrollRange = 0;
    int maxScrollRange = getMaxScrollRange();
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

    if (!hasVelocity(velocityX, velocityY)) {
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

  private int findMultiStepTargetSnapPosition(int velocityX, int velocityY) {
    int viewportSize = mIsVertical ? mSnapHooks.getScrollHeight() : mSnapHooks.getScrollWidth();
    if (viewportSize <= 0) {
      return INVALID_INDEX;
    }
    int velocity = mIsVertical ? velocityY : velocityX;
    int contentOffset = mIsVertical ? mSnapHooks.getScrollY() : mSnapHooks.getScrollX();
    boolean forwardDirection = isForwardFling(velocityX, velocityY);

    // Calculate effective distance.
    int effectiveDistance = calculateEffectiveFlingDistance(velocity, viewportSize);
    int projectedOffset =
        contentOffset + (forwardDirection ? effectiveDistance : -effectiveDistance);

    // Collect directional candidates.
    List<SnapCandidate> directionalCandidates =
        collectDirectionalSnapCandidates(contentOffset, forwardDirection);
    if (directionalCandidates.isEmpty()) {
      return INVALID_INDEX;
    }

    // Sort candidates by distance to current position.
    Collections.sort(directionalCandidates, new Comparator<SnapCandidate>() {
      @Override
      public int compare(SnapCandidate lhs, SnapCandidate rhs) {
        return Integer.compare(Math.abs(lhs.distanceToCurrent), Math.abs(rhs.distanceToCurrent));
      }
    });

    // Find the best candidate.
    SnapCandidate firstCandidate = directionalCandidates.get(0);
    SnapCandidate bestCandidate = firstCandidate;
    int bestDistance = Math.abs(firstCandidate.offset - projectedOffset);
    for (SnapCandidate candidate : directionalCandidates) {
      int snapCount = Math.abs(candidate.position - firstCandidate.position) + 1;
      if (snapCount > mMaxSnapCount) {
        continue;
      }
      int distance = Math.abs(candidate.offset - projectedOffset);
      if (distance < bestDistance) {
        bestDistance = distance;
        bestCandidate = candidate;
      }
    }
    return bestCandidate.position;
  }

  private List<SnapCandidate> collectDirectionalSnapCandidates(
      int contentOffset, boolean forwardDirection) {
    List<SnapCandidate> candidates = new ArrayList<>();
    int minScrollRange = 0;
    int maxScrollRange = getMaxScrollRange();
    SnapCandidate startCandidate = null;
    SnapCandidate endCandidate = null;
    int startCandidateRawOffset = Integer.MIN_VALUE;
    int endCandidateRawOffset = Integer.MAX_VALUE;
    final int childCount = mSnapHooks.getChildrenCount();
    for (int i = 0; i < childCount; i++) {
      View child = mSnapHooks.getChildAtIndex(i);
      if (child == null) {
        continue;
      }
      int position = mSnapHooks.getIndexFromView(child);
      if (position == INVALID_INDEX) {
        continue;
      }
      int itemSnapOffset = getListItemSnapScrollOffset(child);
      int snapOffset = Math.max(minScrollRange, Math.min(itemSnapOffset, maxScrollRange));
      int distanceToCurrent = snapOffset - contentOffset;
      boolean isDirectionalCandidate = (forwardDirection && distanceToCurrent > 0)
          || (!forwardDirection && distanceToCurrent < 0);
      if (!isDirectionalCandidate) {
        continue;
      }
      SnapCandidate candidate = new SnapCandidate(position, snapOffset, distanceToCurrent);
      // Collapse candidates clamped to the same boundary into one representative item.
      if (itemSnapOffset <= minScrollRange) {
        if (startCandidate == null || itemSnapOffset > startCandidateRawOffset) {
          startCandidate = candidate;
          startCandidateRawOffset = itemSnapOffset;
        }
        continue;
      }
      if (itemSnapOffset >= maxScrollRange) {
        if (endCandidate == null || itemSnapOffset < endCandidateRawOffset) {
          endCandidate = candidate;
          endCandidateRawOffset = itemSnapOffset;
        }
        continue;
      }
      candidates.add(candidate);
    }
    // Add at most one candidate for each scroll boundary.
    if (startCandidate != null) {
      candidates.add(startCandidate);
    }
    if (endCandidate != null) {
      candidates.add(endCandidate);
    }
    return candidates;
  }

  private int getMaxScrollRange() {
    return Math.max(0,
        mIsVertical ? mSnapHooks.getContentHeight() - mSnapHooks.getScrollHeight()
                    : mSnapHooks.getContentWidth() - mSnapHooks.getScrollWidth());
  }

  private int calculateEffectiveFlingDistance(int velocity, int viewportSize) {
    // This is not OverScroller's physical fling distance. It is a heuristic distance for
    // choosing snap candidates.
    // First, normalize the velocity above the extra-distance threshold by viewport size:
    //   normalizedVelocity = max(abs(velocity) - 2000, 0) / (viewportSize * 2)
    // For the same velocity, a smaller viewport produces a larger normalized value, while a
    // larger viewport produces a smaller one. For example, with velocity = 3000:
    //   viewportSize = 500  -> normalizedVelocity = 1000 / 1000 = 1
    //   viewportSize = 1000 -> normalizedVelocity = 1000 / 2000 = 0.5
    double normalizedVelocity =
        Math.max(Math.abs(velocity) - EXTRA_DISTANCE_VELOCITY_THRESHOLD, 0.0d)
        / (viewportSize * VELOCITY_TO_VIEWPORT_RATIO);

    // Then, use log1p(x), which means ln(1 + x), to compress high velocities. A linear
    // mapping would double the distance when velocity doubles, making fast flings skip too many
    // items. With ln(1 + x), x=1 -> ln(2)=0.693 / x=2 -> ln(3)=1.099 / x=4 -> ln(5)=1.609 / x=8 ->
    // ln(9)=2.197, so the distance grows without increasing linearly with velocity.
    return (int) Math.round(viewportSize * Math.log1p(normalizedVelocity) * FLING_DISTANCE_GAIN);
  }

  private boolean isForwardFling(int velocityX, int velocityY) {
    if (mIsVertical) {
      return velocityY >= 0;
    } else {
      return velocityX >= 0;
    }
  }

  private static class SnapCandidate {
    final int position;
    final int offset;
    final int distanceToCurrent;

    SnapCandidate(int position, int offset, int distanceToCurrent) {
      this.position = position;
      this.offset = offset;
      this.distanceToCurrent = distanceToCurrent;
    }
  }
}
