// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

import android.graphics.Rect;
import java.util.BitSet;
import java.util.HashMap;
import java.util.Map;

/// FSPSnapshot is a snapshot of FSP.
public class FSPSnapshot {
  private static final int kXProjectionsLen = 256;
  private static final int kYProjectionsLen = 512;

  // X-axis projection bitmap
  private final BitSet mXProjections = new BitSet(kXProjectionsLen);
  // Y-axis projection bitmap
  private final BitSet mYProjections = new BitSet(kYProjectionsLen);

  // X-axis total content projection bitmap
  private final BitSet mXTotalContentProjections = new BitSet(kXProjectionsLen);
  // Y-axis total content projection bitmap
  private final BitSet mYTotalContentProjections = new BitSet(kYProjectionsLen);

  private int mContainerWidth = 0; // Container width.
  private int mContainerHeight = 0; // Container height.
  private long mTotalPresentedContentArea = 0; // Presented effective total area
  private long mTotalContentArea = 0; // Effective total area
  private long mLastChangeTimestampUs = 0; // Timestamp of last content change

  /// X-axis content fill percentage
  private int mContentFillPercentageX = 0;
  /// Y-axis content fill percentage
  private int mContentFillPercentageY = 0;
  /// Total content fill percentage
  /// Formula: (presented content area / content area) * 100
  private int mContentFillPercentageTotalArea = 0;
  /// Percentage of presented content area relative to container area.
  /// Formula: (presented content area / container area) * 100
  private int mContainerFillPercentageContainerArea = 0;

  public long traceCurrentTimestampUs = 0;

  public FSPSnapshot(int containerWidth, int containerHeight, long lastChangeTimestampUs) {
    mContainerWidth = containerWidth;
    mContainerHeight = containerHeight;
    mLastChangeTimestampUs = lastChangeTimestampUs;
  }

  public FSPSnapshot() {
    // Default constructor
  }

  public void fillContentToSnapshot(
      boolean isPresented, Rect rect, long firstPresentedTimestampUs) {
    // If content is invalid, return directly
    if (rect == null || rect.isEmpty() || mContainerWidth == 0 || mContainerHeight == 0) {
      return;
    }

    int w = mContainerWidth;
    int h = mContainerHeight;

    // Optimization for edge case: Ignore if content is outside container bounds
    if (rect.left > w || rect.top > h || rect.right <= 0 || rect.bottom <= 0) {
      return;
    }

    // X/Y axis projection algorithm: "Flatten" the 2D rect area onto 1D X and Y
    // axes respectively, and mark the covered intervals
    int minX = Math.max(0, rect.left);
    int maxX = Math.min(w, rect.right);

    int minY = Math.max(0, rect.top);
    int maxY = Math.min(h, rect.bottom);

    // 1. Calculate pixel area of the content rectangle.
    int visibleContentArea = (maxX - minX) * (maxY - minY);
    mTotalContentArea += visibleContentArea;
    if (isPresented) {
      // Accumulate presented effective total area
      mTotalPresentedContentArea += visibleContentArea;
    }

    // 2. Update X,Y projections
    int minProjX = (int) ((long) minX * kXProjectionsLen / w);
    minProjX = Math.min(minProjX, kXProjectionsLen - 1);
    int maxProjX = (int) ((long) maxX * kXProjectionsLen / w);
    maxProjX = Math.min(maxProjX, kXProjectionsLen - 1);

    int minProjY = (int) ((long) minY * kYProjectionsLen / h);
    minProjY = Math.min(minProjY, kYProjectionsLen - 1);
    int maxProjY = (int) ((long) maxY * kYProjectionsLen / h);
    maxProjY = Math.min(maxProjY, kYProjectionsLen - 1);

    mXTotalContentProjections.set(minProjX, maxProjX + 1);
    if (isPresented) {
      mXProjections.set(minProjX, maxProjX + 1);
    }

    mYTotalContentProjections.set(minProjY, maxProjY + 1);
    if (isPresented) {
      mYProjections.set(minProjY, maxProjY + 1);
    }

    // Update snapshot's last change timestamp
    if (firstPresentedTimestampUs > mLastChangeTimestampUs) {
      mLastChangeTimestampUs = firstPresentedTimestampUs;
    }
  }

  // Getters and setters
  public int getContainerWidth() {
    return mContainerWidth;
  }

  public int getContainerHeight() {
    return mContainerHeight;
  }

  public long getTotalPresentedContentArea() {
    return mTotalPresentedContentArea;
  }

  public void setTotalPresentedContentArea(long totalPresentedContentArea) {
    mTotalPresentedContentArea = totalPresentedContentArea;
  }

  public long getTotalContentArea() {
    return mTotalContentArea;
  }

  public long getContainerArea() {
    return (long) mContainerWidth * mContainerHeight;
  }

  public void setTotalContentArea(long totalContentArea) {
    mTotalContentArea = totalContentArea;
  }

  public long getLastChangeTimestampUs() {
    return mLastChangeTimestampUs;
  }

  public void setLastChangeTimestampUs(long lastChangeTimestampUs) {
    mLastChangeTimestampUs = lastChangeTimestampUs;
  }

  public int getContentFillPercentageX() {
    return mContentFillPercentageX;
  }

  public void setContentFillPercentageX(int contentFillPercentageX) {
    mContentFillPercentageX = contentFillPercentageX;
  }

  public int getContentFillPercentageY() {
    return mContentFillPercentageY;
  }

  public void setContentFillPercentageY(int contentFillPercentageY) {
    mContentFillPercentageY = contentFillPercentageY;
  }

  public int getContentFillPercentageTotalArea() {
    return mContentFillPercentageTotalArea;
  }

  public void setContentFillPercentageTotalArea(int contentFillPercentageTotalArea) {
    mContentFillPercentageTotalArea = contentFillPercentageTotalArea;
  }

  public int getContainerFillPercentageContainerArea() {
    return mContainerFillPercentageContainerArea;
  }

  public void setContainerFillPercentageContainerArea(int containerFillPercentageContainerArea) {
    mContainerFillPercentageContainerArea = containerFillPercentageContainerArea;
  }

  public BitSet getXProjections() {
    return mXProjections;
  }

  public BitSet getYProjections() {
    return mYProjections;
  }

  public BitSet getXTotalContentProjections() {
    return mXTotalContentProjections;
  }

  public BitSet getYTotalContentProjections() {
    return mYTotalContentProjections;
  }

  /// Just for Debug
  public Map<String, String> toMap() {
    HashMap<String, String> map = new HashMap<>();
    map.put("ContainerSize", String.valueOf(mContainerWidth * mContainerHeight));
    map.put("mTotalPresentedContentArea", String.valueOf(mTotalPresentedContentArea));
    map.put("mTotalContentArea", String.valueOf(mTotalContentArea));
    // fill percentage
    map.put("mContentFillPercentageX", String.valueOf(mContentFillPercentageX));
    map.put("mContentFillPercentageY", String.valueOf(mContentFillPercentageY));
    map.put("mContentFillPercentageTotalArea", String.valueOf(mContentFillPercentageTotalArea));
    map.put("mContainerFillPercentageContainerArea",
        String.valueOf(mContainerFillPercentageContainerArea));
    // trace timestamp
    map.put("traceCurrentTimestampUs", String.valueOf(traceCurrentTimestampUs));
    map.put("mLastChangeTimestampUs", String.valueOf(mLastChangeTimestampUs));
    return map;
  }
}
