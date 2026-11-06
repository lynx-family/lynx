// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import android.os.Debug;
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
  private static final String TAG = "LynxMemoryUsageResult";
  private static final int GLOBAL_VALUE_COUNT = 6;
  private static final int INSTANCE_VALUE_COUNT = 7;
  private static final int INSTANCE_STRING_COUNT = 3;

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

  private LynxGlobalMemoryUsageResult(long collectionStartMs,
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

  @NonNull
  static LynxGlobalMemoryUsageResult build(long collectionStartMs,
      @NonNull LynxMemoryCollectionStatus collectionStatus, long collectionDurationMs,
      long collectionTimeoutMs, int expectedInstanceCount, long appBytes,
      @NonNull List<LynxInstanceMemoryUsage> instances) {
    long elementBytes = 0;
    long elementNodeCount = 0;
    long viewBytes = 0;
    long mainThreadRuntimeBytes = 0;
    long backgroundThreadRuntimeBytes = 0;
    for (LynxInstanceMemoryUsage instance : instances) {
      elementBytes += instance.getElementBytes();
      elementNodeCount += instance.getElementNodeCount();
      viewBytes += instance.getViewBytes();
      mainThreadRuntimeBytes += instance.getMainThreadRuntimeBytes();
      backgroundThreadRuntimeBytes += instance.getBackgroundThreadRuntimeBytes();
    }
    long totalBytes =
        elementBytes + viewBytes + mainThreadRuntimeBytes + backgroundThreadRuntimeBytes;
    return create(collectionStartMs, collectionStatus, collectionDurationMs, collectionTimeoutMs,
        expectedInstanceCount, totalBytes, appBytes, elementBytes, elementNodeCount, viewBytes,
        mainThreadRuntimeBytes, backgroundThreadRuntimeBytes, instances);
  }

  @NonNull
  static LynxGlobalMemoryUsageResult fromNative(long collectionStartMs, long collectionDurationMs,
      long collectionTimeoutMs, @NonNull long[] globalValues, @NonNull long[] instanceValues,
      @NonNull String[] instanceStrings) {
    if (globalValues.length < GLOBAL_VALUE_COUNT) {
      return build(collectionStartMs, LynxMemoryCollectionStatus.COMPLETED, collectionDurationMs,
          collectionTimeoutMs, 0, sampleAppBytes(), Collections.emptyList());
    }
    int instanceCount = Math.min(instanceValues.length / INSTANCE_VALUE_COUNT,
        instanceStrings.length / INSTANCE_STRING_COUNT);
    ArrayList<LynxInstanceMemoryUsage> instances = new ArrayList<>(instanceCount);
    for (int i = 0; i < instanceCount; ++i) {
      int valueIndex = i * INSTANCE_VALUE_COUNT;
      int stringIndex = i * INSTANCE_STRING_COUNT;
      instances.add(new LynxInstanceMemoryUsage((int) instanceValues[valueIndex],
          instanceStrings[stringIndex], instanceStrings[stringIndex + 1],
          instanceValues[valueIndex + 1], instanceValues[valueIndex + 2],
          instanceValues[valueIndex + 3], instanceValues[valueIndex + 4], null,
          instanceValues[valueIndex + 5], instanceValues[valueIndex + 6],
          instanceStrings[stringIndex + 2]));
    }
    long appBytes = sampleAppBytes();
    return create(collectionStartMs, LynxMemoryCollectionStatus.COMPLETED, collectionDurationMs,
        collectionTimeoutMs, instanceCount, globalValues[0], appBytes, globalValues[1],
        globalValues[2], globalValues[3], globalValues[4], globalValues[5], instances);
  }

  private static LynxGlobalMemoryUsageResult create(long collectionStartMs,
      @NonNull LynxMemoryCollectionStatus collectionStatus, long collectionDurationMs,
      long collectionTimeoutMs, int expectedInstanceCount, long totalBytes, long appBytes,
      long elementBytes, long elementNodeCount, long viewBytes, long mainThreadRuntimeBytes,
      long backgroundThreadRuntimeBytes, @NonNull List<LynxInstanceMemoryUsage> instances) {
    double ratioToApp = appBytes > 0 ? (double) totalBytes / (double) appBytes : 0;
    return new LynxGlobalMemoryUsageResult(collectionStartMs, collectionStatus,
        collectionDurationMs, collectionTimeoutMs, expectedInstanceCount, instances.size(),
        totalBytes, appBytes, ratioToApp, elementBytes, elementNodeCount, viewBytes,
        mainThreadRuntimeBytes, backgroundThreadRuntimeBytes, instances);
  }

  private static long sampleAppBytes() {
    Debug.MemoryInfo memoryInfo = new Debug.MemoryInfo();
    Debug.getMemoryInfo(memoryInfo);
    return Math.max(0L, memoryInfo.getTotalPss()) * 1024L;
  }

  /** Wall-clock collection start time in milliseconds. */
  public long getCollectionStartMs() {
    return mCollectionStartMs;
  }

  /** Whether the native global snapshot completed. */
  @NonNull
  public LynxMemoryCollectionStatus getCollectionStatus() {
    return mCollectionStatus;
  }

  /** Elapsed collection time in milliseconds. */
  public long getCollectionDurationMs() {
    return mCollectionDurationMs;
  }

  /** Timeout metadata retained for API compatibility. */
  public long getCollectionTimeoutMs() {
    return mCollectionTimeoutMs;
  }

  /** Number of active native instances in the snapshot. */
  public int getExpectedInstanceCount() {
    return mExpectedInstanceCount;
  }

  /** Number of instance results included in this result. */
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

  /** Aggregated element bytes across active instances. */
  public long getElementBytes() {
    return mElementBytes;
  }

  /** Aggregated element node count across active instances. */
  public long getElementNodeCount() {
    return mElementNodeCount;
  }

  /** Aggregated UI/view bytes across active instances. */
  public long getViewBytes() {
    return mViewBytes;
  }

  /** Aggregated main-thread runtime bytes across active instances. */
  public long getMainThreadRuntimeBytes() {
    return mMainThreadRuntimeBytes;
  }

  /** All active BTS VM heaps, counted once by native VM identity. */
  public long getBackgroundThreadRuntimeBytes() {
    return mBackgroundThreadRuntimeBytes;
  }

  /** Active instance list, sorted by instance totalBytes descending. */
  @NonNull
  public List<LynxInstanceMemoryUsage> getInstances() {
    return mInstances;
  }
}
