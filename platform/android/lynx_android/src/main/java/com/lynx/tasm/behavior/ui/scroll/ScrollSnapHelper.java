// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.scroll;

import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.utils.PixelUtils;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * Item-snap target calculator.
 *
 * <p>The container normalizes coordinates before calling this helper: a positive velocity means
 * that the current offset increases. View lookup, RTL conversion, snap events, and scroll
 * animation belong to the container adapter.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class ScrollSnapHelper {
  private static final int INVALID_INDEX = -1;

  private static final String PARAM_FACTOR = "factor";
  private static final String PARAM_OFFSET = "offset";
  private static final String PARAM_MAX_SNAP_COUNT = "maxSnapCount";
  private static final int DEFAULT_MAX_SNAP_COUNT = 1;
  private static final double EXTRA_DISTANCE_VELOCITY_THRESHOLD = 2000.0d;
  private static final double VELOCITY_TO_VIEWPORT_RATIO = 2.0d;

  /**
   * Provides scroll and item geometry without exposing Android {@code View}s.
   */
  public interface ScrollContainerHooks {
    /** Returns the current normalized main-axis offset. */
    int getCurrentOffset();

    /** Returns the minimum valid normalized main-axis offset. */
    int getMinOffset();

    /** Returns the maximum valid normalized main-axis offset. */
    int getMaxOffset();

    /** Returns the visible length of the scroll container on the normalized main axis. */
    int getViewportSize();

    /**
     * Captures snap-item geometry once for one target-selection pass.
     */
    List<SnapItem> getSnapItems();

    /**
     * Resolves an adjacent item only when a virtualized container has no candidate in the fling
     * direction. A normal scroll-view can return {@code null}.
     *
     * @param index reported index of the visible item nearest to the fling direction
     * @param forward whether the normalized offset is increasing
     */
    @Nullable SnapItem getAdjacentSnapItem(int index, boolean forward);

    /**
     * Reports the selected snap target on the normalized main axis before the container scrolls.
     *
     * @param index selected snap-item index, or a negative value when no target exists
     * @param currentOffset current normalized main-axis offset
     * @param targetOffset selected normalized main-axis offset
     */
    void willSnapTo(int index, int currentOffset, int targetOffset);

    /** Reports an invalid item-snap configuration. */
    void handleError(String message);
  }

  /**
   * Geometry of one item on the normalized scroll axis.
   */
  public static final class SnapItem {
    private final int mIndex;
    private final int mStart;
    private final int mSize;

    /**
     * Creates an item's normalized main-axis geometry.
     *
     * @param index index reported by the snap event
     * @param start item start offset on the normalized main axis
     * @param size item length on the normalized main axis
     */
    public SnapItem(int index, int start, int size) {
      this.mIndex = index;
      this.mStart = start;
      this.mSize = size;
    }
  }

  /** The selected item and its normalized target offset. */
  public static final class SnapTarget {
    /** Index of the selected snap item, or a negative value when no target exists. */
    public final int index;

    /** Normalized main-axis offset at which the selected item is aligned. */
    public final int targetOffset;

    private SnapTarget(int index, int targetOffset) {
      this.index = index;
      this.targetOffset = targetOffset;
    }

    /** Returns whether a valid snap target was found. */
    public boolean hasTarget() {
      return index != INVALID_INDEX;
    }
  }

  private final double mAlignmentFactor;
  private final int mAlignmentOffset;
  private final int mMaxSnapCount;
  private final ScrollContainerHooks mHooks;

  /**
   * Creates a helper from an enabled item-snap prop.
   *
   * <p>A null or empty item-snap prop disables snapping and must be handled by the owner.</p>
   *
   * @param params item-snap prop
   * @param hooks source of normalized scroll geometry, snap-item candidates, target reporting,
   *     and error reporting
   */
  public ScrollSnapHelper(ReadableMap params, ScrollContainerHooks hooks) {
    this(params.getDouble(PARAM_FACTOR), params.getInt(PARAM_OFFSET, 0),
        params.getInt(PARAM_MAX_SNAP_COUNT, DEFAULT_MAX_SNAP_COUNT), hooks);
  }

  /**
   * Creates a helper for one scroll container.
   *
   * @param alignmentFactor alignment within the viewport. {@code 0} aligns the item start with
   *     the viewport start, and {@code 1} aligns the item end with the viewport end. The caller
   *     must provide a value in the range [0, 1]; invalid values use {@code 0} after reporting
   *     an error.
   * @param alignmentOffset offset, in dip, added to the calculated target offset
   * @param maxSnapCount maximum number of snap items, including the first directional candidate,
   *     selectable by one non-zero-velocity fling; values below one use one after reporting an
   *     error
   * @param hooks source of normalized scroll geometry, snap-item candidates, target reporting,
   *     and error reporting
   */
  public ScrollSnapHelper(
      double alignmentFactor, int alignmentOffset, int maxSnapCount, ScrollContainerHooks hooks) {
    if (alignmentFactor < 0 || alignmentFactor > 1) {
      hooks.handleError(
          "item-snap invalid! The factor should be constrained to the range of [0,1].");
      alignmentFactor = 0;
    }
    if (maxSnapCount < DEFAULT_MAX_SNAP_COUNT) {
      hooks.handleError("item-snap invalid! The maxSnapCount should be greater than 0.");
      maxSnapCount = DEFAULT_MAX_SNAP_COUNT;
    }
    mAlignmentFactor = alignmentFactor;
    mAlignmentOffset = (int) PixelUtils.dipToPx(alignmentOffset);
    mMaxSnapCount = maxSnapCount;
    mHooks = hooks;
  }

  /**
   * Finds the target for a normalized main-axis velocity.
   *
   * @param velocity normalized velocity; a positive value moves toward increasing offsets, a
   *     negative value moves toward decreasing offsets, and zero selects the nearest candidate
   * @return the selected index and normalized target offset, or a result without a target. The
   *     result is reported through {@link ScrollContainerHooks#willSnapTo(int, int, int)} before
   *     this method returns.
   */
  public SnapTarget findSnapTarget(int velocity) {
    SnapContext context = updateSnapContext();
    SnapTarget snapTarget;
    if (context.mViewportSize <= 0 || context.mSnapItems.isEmpty()) {
      // Invalid viewport size or No valid snap items.
      snapTarget = noTarget(context.mCurrentOffset);
    } else {
      SnapItem snapItem = null;
      if (velocity != 0 && mMaxSnapCount > DEFAULT_MAX_SNAP_COUNT) {
        // Multi-step snap
        snapItem = findMultiStepTarget(context, velocity);
      }
      if (snapItem == null) {
        // Single-step snap
        snapItem = findSingleStepTarget(context, velocity);
      }
      if (snapItem != null) {
        int targetOffset =
            clamp(getSnapOffset(snapItem, context), context.mMinOffset, context.mMaxOffset);
        snapTarget = new SnapTarget(snapItem.mIndex, targetOffset);
      } else {
        snapTarget = noTarget(context.mCurrentOffset);
      }
    }

    mHooks.willSnapTo(snapTarget.index, context.mCurrentOffset, snapTarget.targetOffset);
    return snapTarget;
  }

  private SnapContext updateSnapContext() {
    List<SnapItem> snapItems = new ArrayList<>();
    for (SnapItem item : mHooks.getSnapItems()) {
      // Keep a zero-sized item: although its own layout size is zero, overflowing descendants can
      // still make its start offset a valid snap point. Negative geometry is invalid.
      if (item != null && item.mSize >= 0) {
        snapItems.add(item);
      }
    }
    int minOffset = mHooks.getMinOffset();
    int maxOffset = Math.max(minOffset, mHooks.getMaxOffset());
    return new SnapContext(
        mHooks.getCurrentOffset(), minOffset, maxOffset, mHooks.getViewportSize(), snapItems);
  }

  @Nullable
  private SnapItem findSingleStepTarget(SnapContext context, int velocity) {
    SnapCandidate closestCandidateBefore = null;
    SnapCandidate closestCandidateAfter = null;
    SnapCandidate clampedCandidateBefore = null;
    SnapCandidate clampedCandidateAfter = null;
    List<SnapCandidate> candidates = new ArrayList<>();
    int distanceBefore = Integer.MIN_VALUE;
    int distanceAfter = Integer.MAX_VALUE;

    // Collect one representative for each scroll boundary before choosing the nearest candidates.
    // An item exactly at the current offset remains eligible on both sides during that selection.
    for (SnapItem item : context.mSnapItems) {
      int rawOffset = getSnapOffset(item, context);
      int offset = clamp(rawOffset, context.mMinOffset, context.mMaxOffset);
      SnapCandidate candidate =
          new SnapCandidate(item, rawOffset, offset, offset - context.mCurrentOffset);

      if (rawOffset <= context.mMinOffset) {
        // Collapse all start-boundary candidates into the item whose raw offset is nearest to the
        // start boundary. This includes an item already aligned with that boundary.
        if (clampedCandidateBefore == null || rawOffset > clampedCandidateBefore.mRawOffset) {
          clampedCandidateBefore = candidate;
        }
      } else if (rawOffset >= context.mMaxOffset) {
        // Collapse all end-boundary candidates into the item whose raw offset is nearest to the
        // end boundary. This includes an item already aligned with that boundary.
        if (clampedCandidateAfter == null || rawOffset < clampedCandidateAfter.mRawOffset) {
          clampedCandidateAfter = candidate;
        }
      } else {
        candidates.add(candidate);
      }
    }

    // Add at most one candidate for each scroll boundary. Selecting nearest candidates after
    // this collapse keeps the returned item and its clamped target offset consistent.
    if (clampedCandidateBefore != null) {
      candidates.add(clampedCandidateBefore);
    }
    if (clampedCandidateAfter != null) {
      candidates.add(clampedCandidateAfter);
    }

    for (SnapCandidate candidate : candidates) {
      int distance = candidate.mDistanceToCurrent;
      if (distance <= 0 && distance > distanceBefore) {
        // Choose the candidate whose clamped snap offset is nearest before the current offset.
        distanceBefore = distance;
        closestCandidateBefore = candidate;
      }
      if (distance >= 0 && distance < distanceAfter) {
        // Choose the candidate whose clamped snap offset is nearest after the current offset.
        distanceAfter = distance;
        closestCandidateAfter = candidate;
      }
    }

    boolean forward = velocity > 0;
    if (velocity == 0) {
      // No velocity, choose the nearest candidate.
      if (closestCandidateAfter != null && closestCandidateBefore != null) {
        // Keep the list behavior: choose "before" when the distances tie.
        if (Math.abs(distanceAfter) < Math.abs(distanceBefore)) {
          return closestCandidateAfter.mItem;
        } else {
          return closestCandidateBefore.mItem;
        }
      } else if (closestCandidateAfter != null) {
        return closestCandidateAfter.mItem;
      } else if (closestCandidateBefore != null) {
        return closestCandidateBefore.mItem;
      }
    } else {
      // With velocity, choose the first candidate in the fling direction rather than the nearest
      // candidate on either side.
      if (forward && closestCandidateAfter != null) {
        return closestCandidateAfter.mItem;
      } else if (!forward && closestCandidateBefore != null) {
        return closestCandidateBefore.mItem;
      }
    }

    // This fallback is needed only when a virtualized container has no attached candidate in the
    // fling direction. Non-virtual scroll-views normally return null here.
    SnapCandidate visibleItem = forward ? closestCandidateBefore : closestCandidateAfter;
    if (visibleItem != null) {
      return mHooks.getAdjacentSnapItem(visibleItem.mItem.mIndex, forward);
    } else {
      return null;
    }
  }

  @Nullable
  private SnapItem findMultiStepTarget(SnapContext context, int velocity) {
    boolean forward = velocity > 0;
    // This is a target-selection heuristic, not the platform scroller's physical fling distance.
    // It projects a farther target for faster flings while keeping the distance viewport-relative.
    int effectiveDistance = calculateEffectiveFlingDistance(velocity, context.mViewportSize);
    int projectedOffset =
        context.mCurrentOffset + (forward ? effectiveDistance : -effectiveDistance);

    // Only candidates strictly in the fling direction are eligible for a multi-step snap.
    List<SnapCandidate> candidates = collectDirectionalCandidates(context, forward);
    if (candidates.isEmpty()) {
      return null;
    }

    // The nearest directional candidate establishes the first snap step.
    Collections.sort(candidates, new Comparator<SnapCandidate>() {
      @Override
      public int compare(SnapCandidate lhs, SnapCandidate rhs) {
        return Integer.compare(Math.abs(lhs.mDistanceToCurrent), Math.abs(rhs.mDistanceToCurrent));
      }
    });

    SnapCandidate first = candidates.get(0);
    SnapCandidate best = first;
    int bestDistance = Math.abs(first.mOffset - projectedOffset);
    for (SnapCandidate candidate : candidates) {
      // The first directional candidate itself counts as one snap step, hence the +1. Use the
      // helper's raw item indices rather than candidate-list positions. Raw indices may have
      // gaps for non-snapping children such as bounce views.
      int snapCount = Math.abs(candidate.mItem.mIndex - first.mItem.mIndex) + 1;
      if (snapCount > mMaxSnapCount) {
        continue;
      }
      int distance = Math.abs(candidate.mOffset - projectedOffset);
      // Among candidates within maxSnapCount, choose the one nearest to the projected offset.
      if (distance < bestDistance) {
        best = candidate;
        bestDistance = distance;
      }
    }
    return best.mItem;
  }

  private List<SnapCandidate> collectDirectionalCandidates(SnapContext context, boolean forward) {
    List<SnapCandidate> candidates = new ArrayList<>();
    SnapCandidate startCandidate = null;
    SnapCandidate endCandidate = null;

    for (SnapItem item : context.mSnapItems) {
      int rawOffset = getSnapOffset(item, context);
      int offset = clamp(rawOffset, context.mMinOffset, context.mMaxOffset);
      int distance = offset - context.mCurrentOffset;
      if ((forward && distance <= 0) || (!forward && distance >= 0)) {
        continue;
      }
      SnapCandidate candidate = new SnapCandidate(item, rawOffset, offset, distance);
      if (rawOffset <= context.mMinOffset) {
        // Collapse all start-boundary candidates into one representative item.
        if (startCandidate == null || rawOffset > startCandidate.mRawOffset) {
          startCandidate = candidate;
        }
      } else if (rawOffset >= context.mMaxOffset) {
        // Collapse all end-boundary candidates into one representative item.
        if (endCandidate == null || rawOffset < endCandidate.mRawOffset) {
          endCandidate = candidate;
        }
      } else {
        // Middle candidates do not collapse with other items, so add them directly.
        candidates.add(candidate);
      }
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

  private int getSnapOffset(SnapItem item, SnapContext context) {
    // factor = 0 aligns item start to viewport start; factor = 1 aligns item end to viewport end.
    return (int) (item.mStart - (context.mViewportSize - item.mSize) * mAlignmentFactor
        + mAlignmentOffset);
  }

  private int calculateEffectiveFlingDistance(int velocity, int viewportSize) {
    // Normalize the velocity beyond the extra-distance threshold by viewport size. A smaller
    // viewport therefore projects a farther target for the same velocity.
    // normalizedVelocity = max(abs(velocity) - 2000, 0) / (viewportSize * 2)
    double normalizedVelocity =
        Math.max(Math.abs(velocity) - EXTRA_DISTANCE_VELOCITY_THRESHOLD, 0.0d)
        / (viewportSize * VELOCITY_TO_VIEWPORT_RATIO);

    // log1p compresses high velocities so the projected distance grows smoothly instead of
    // linearly, preventing a very fast fling from skipping too many items.
    return (int) Math.round(viewportSize * Math.log1p(normalizedVelocity));
  }

  private static int clamp(int value, int min, int max) {
    return Math.max(min, Math.min(value, max));
  }

  private static SnapTarget noTarget(int currentOffset) {
    return new SnapTarget(INVALID_INDEX, currentOffset);
  }

  /** Immutable scroll context captured once for one release or fling. */
  private static final class SnapContext {
    private final int mCurrentOffset;
    private final int mMinOffset;
    private final int mMaxOffset;
    private final int mViewportSize;
    private final List<SnapItem> mSnapItems;

    private SnapContext(int currentOffset, int minOffset, int maxOffset, int viewportSize,
        List<SnapItem> snapItems) {
      this.mCurrentOffset = currentOffset;
      this.mMinOffset = minOffset;
      this.mMaxOffset = maxOffset;
      this.mViewportSize = viewportSize;
      this.mSnapItems = Collections.unmodifiableList(snapItems);
    }
  }

  private static final class SnapCandidate {
    final SnapItem mItem;
    final int mRawOffset;
    final int mOffset;
    final int mDistanceToCurrent;

    SnapCandidate(SnapItem item, int rawOffset, int offset, int distanceToCurrent) {
      this.mItem = item;
      this.mRawOffset = rawOffset;
      this.mOffset = offset;
      this.mDistanceToCurrent = distanceToCurrent;
    }
  }
}
