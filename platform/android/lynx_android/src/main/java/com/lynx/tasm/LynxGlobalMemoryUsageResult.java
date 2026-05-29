// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Immutable typed result returned by LynxMemoryUsageQuery's global memory query API.
 *
 * <p>Use the individual getters to read only the fields needed by the caller. Per-instance details
 * are exposed through an unmodifiable list from {@link #getInstances()}.
 */
public final class LynxGlobalMemoryUsageResult {
  private final long mCollectionStartMs;
  @NonNull private final LynxMemoryCollectionStatus mCollectionStatus;
  private final long mCollectionDurationMs;
  private final long mCollectionTimeoutMs;
  private final int mExpectedInstanceCount;
  private final int mCompletedInstanceCount;
  private final long mTotalBytes;
  private final long mAppBytes;
  private final double mRatioToApp;
  private final long mElementBytes;
  private final long mElementNodeCount;
  private final long mViewBytes;
  private final long mMainThreadRuntimeBytes;
  private final long mBackgroundThreadRuntimeBytes;
  @NonNull private final List<LynxInstanceMemoryUsage> mInstances;

  LynxGlobalMemoryUsageResult(long collectionStartMs,
      @NonNull LynxMemoryCollectionStatus collectionStatus, long collectionDurationMs,
      long collectionTimeoutMs, int expectedInstanceCount, int completedInstanceCount,
      long totalBytes, long appBytes, double ratioToApp, long elementBytes, long elementNodeCount,
      long viewBytes, long mainThreadRuntimeBytes, long backgroundThreadRuntimeBytes,
      @NonNull List<LynxInstanceMemoryUsage> instances) {
    mCollectionStartMs = collectionStartMs;
    mCollectionStatus = collectionStatus;
    mCollectionDurationMs = collectionDurationMs;
    mCollectionTimeoutMs = collectionTimeoutMs;
    mExpectedInstanceCount = expectedInstanceCount;
    mCompletedInstanceCount = completedInstanceCount;
    mTotalBytes = totalBytes;
    mAppBytes = appBytes;
    mRatioToApp = ratioToApp;
    mElementBytes = elementBytes;
    mElementNodeCount = elementNodeCount;
    mViewBytes = viewBytes;
    mMainThreadRuntimeBytes = mainThreadRuntimeBytes;
    mBackgroundThreadRuntimeBytes = backgroundThreadRuntimeBytes;
    ArrayList<LynxInstanceMemoryUsage> instancesCopy = new ArrayList<>(instances);
    Collections.sort(instancesCopy, (a, b) -> Long.compare(b.getTotalBytes(), a.getTotalBytes()));
    mInstances = Collections.unmodifiableList(instancesCopy);
  }

  /** Wall-clock collection start time in milliseconds. */
  public long getCollectionStartMs() {
    return mCollectionStartMs;
  }

  /** Whether all expected fetchers completed or timeout produced a partial result. */
  @NonNull
  public LynxMemoryCollectionStatus getCollectionStatus() {
    return mCollectionStatus;
  }

  /** Elapsed collection time in milliseconds. */
  public long getCollectionDurationMs() {
    return mCollectionDurationMs;
  }

  /** Fixed timeout used by the collection request. */
  public long getCollectionTimeoutMs() {
    return mCollectionTimeoutMs;
  }

  /** Number of live fetchers captured at request start. */
  public int getExpectedInstanceCount() {
    return mExpectedInstanceCount;
  }

  /** Number of completed instance results included in this result. */
  public int getCompletedInstanceCount() {
    return mCompletedInstanceCount;
  }

  /** Global Lynx-attributed total bytes. */
  public long getTotalBytes() {
    return mTotalBytes;
  }

  /** Current app physical footprint sampled when the global result is built. */
  public long getAppBytes() {
    return mAppBytes;
  }

  /** totalBytes divided by appBytes. Zero when appBytes is unavailable. */
  public double getRatioToApp() {
    return mRatioToApp;
  }

  /** Aggregated element bytes across completed instances. */
  public long getElementBytes() {
    return mElementBytes;
  }

  /** Aggregated element node count across completed instances. */
  public long getElementNodeCount() {
    return mElementNodeCount;
  }

  /** Aggregated UI/view bytes across completed instances. */
  public long getViewBytes() {
    return mViewBytes;
  }

  /** Aggregated main-thread runtime bytes across completed instances. */
  public long getMainThreadRuntimeBytes() {
    return mMainThreadRuntimeBytes;
  }

  /** Aggregated background runtime bytes with shared group ids deduplicated. */
  public long getBackgroundThreadRuntimeBytes() {
    return mBackgroundThreadRuntimeBytes;
  }

  /** Completed instance list, sorted by instance totalBytes descending. */
  @NonNull
  public List<LynxInstanceMemoryUsage> getInstances() {
    return mInstances;
  }
}
