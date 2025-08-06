// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing;

import androidx.annotation.RestrictTo;

/**
 * Base class for all snapshot variants.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public abstract class FSPSnapshot {
  protected int mLastChangeUISign = FSPResult.UNKNOWN_UI_SIGN;
  protected long mLastChangeTimestampNs;
  protected int mContainerWidth;
  protected int mContainerHeight;

  private static int MAX_CONTAINER_SIZE = 65535;

  public int getLastChangeUISign() {
    return mLastChangeUISign;
  }

  public long getLastChangeTimestampNs() {
    return mLastChangeTimestampNs;
  }

  public int getContainerWidth() {
    return mContainerWidth;
  }

  public int getContainerHeight() {
    return mContainerHeight;
  }

  public FSPResult getResult() {
    return new FSPResult(
        true, mLastChangeUISign, mLastChangeTimestampNs, mContainerWidth, mContainerHeight);
  }

  /**
   * Reset the snapshot.
   */
  public void reset() {
    this.mContainerWidth = 0;
    this.mContainerHeight = 0;
    mLastChangeTimestampNs = 0;
    mLastChangeUISign = FSPResult.UNKNOWN_UI_SIGN;
    resetImpl();
  }

  /**
   * Update the container size.
   *
   * @param containerWidth
   * @param containerHeight
   */
  public void setContainerSize(int containerWidth, int containerHeight) {
    if (containerWidth > 0 && containerHeight > 0 && containerWidth <= MAX_CONTAINER_SIZE
        && containerHeight <= MAX_CONTAINER_SIZE) {
      this.mContainerWidth = containerWidth;
      this.mContainerHeight = containerHeight;
    } else {
      this.mContainerWidth = 0;
      this.mContainerHeight = 0;
    }
  }

  /**
   * Update the timestamp and UI sign if the change timestamp is newer.
   *
   * @param changeTimestamp
   * @param uiSign
   */
  public void updateTimestamp(long changeTimestampNs, int uiSign) {
    if (changeTimestampNs >= mLastChangeTimestampNs) {
      mLastChangeUISign = uiSign;
      mLastChangeTimestampNs = changeTimestampNs;
    }
  }

  /**
   * Reset implementation-specific data.
   */
  protected abstract void resetImpl();
}
