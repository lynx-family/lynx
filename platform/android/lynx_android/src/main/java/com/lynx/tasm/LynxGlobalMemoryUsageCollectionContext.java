// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.LynxConsumer;
import java.util.ArrayList;
import java.util.List;

/**
 * Tracks one in-flight global collection and owns the final callback fan-out.
 *
 * <p>Instances may finish successfully, report no data, or miss the timeout. The context counts
 * every finished fetcher toward completion, but only non-null instance snapshots contribute to the
 * final memory totals.
 */
class LynxGlobalMemoryUsageCollectionContext {
  private static final String TAG = "LynxMemoryUsageContext";

  private final long mCollectionStartMs;
  private final long mCollectionTimeoutMs;
  private final int mExpectedInstanceCount;
  private final ArrayList<LynxGlobalMemoryUsageCallback> mCallbacks = new ArrayList<>();
  private final ArrayList<LynxInstanceMemoryUsage> mInstances = new ArrayList<>();
  @Nullable private LynxConsumer<LynxGlobalMemoryUsageCollectionContext> mFinishHandler;
  private int mReceivedFetchResultCount;
  private boolean mFinished;

  LynxGlobalMemoryUsageCollectionContext(long collectionStartMs, long collectionTimeoutMs,
      int expectedInstanceCount, @NonNull LynxGlobalMemoryUsageCallback callback) {
    mCollectionStartMs = collectionStartMs;
    mCollectionTimeoutMs = collectionTimeoutMs;
    mExpectedInstanceCount = expectedInstanceCount;
    addCallback(callback);
  }

  void setFinishHandler(
      @Nullable LynxConsumer<LynxGlobalMemoryUsageCollectionContext> finishHandler) {
    mFinishHandler = finishHandler;
  }

  void addCallback(@Nullable LynxGlobalMemoryUsageCallback callback) {
    if (mFinished || callback == null) {
      return;
    }
    // Extra public queries issued while this context is pending share the same final result.
    mCallbacks.add(callback);
  }

  void didReceiveInstanceResult(@Nullable LynxInstanceMemoryUsage result) {
    if (mFinished) {
      return;
    }
    // A null result still represents one fetcher completing. This prevents a failed per-instance
    // fetch from blocking the whole global query.
    mReceivedFetchResultCount++;
    if (result != null) {
      mInstances.add(result);
    }
    boolean shouldFinish = mReceivedFetchResultCount >= mExpectedInstanceCount;
    if (shouldFinish) {
      finishWithStatus(LynxMemoryCollectionStatus.COMPLETED);
    }
  }

  void finishWithStatus(@NonNull LynxMemoryCollectionStatus status) {
    if (mFinished) {
      return;
    }
    mFinished = true;

    LynxGlobalMemoryUsageResult result = LynxGlobalMemoryUsageResult.build(mCollectionStartMs,
        status, LynxGlobalMemoryUsageCollector.nowMs() - mCollectionStartMs, mCollectionTimeoutMs,
        mExpectedInstanceCount, LynxGlobalMemoryUsageCollector.sampleAppBytes(), mInstances);
    // Copy and clear callbacks before invoking app code so reentrant queries cannot mutate the
    // callback list being delivered.
    List<LynxGlobalMemoryUsageCallback> callbacks = new ArrayList<>(mCallbacks);
    mCallbacks.clear();
    LynxConsumer<LynxGlobalMemoryUsageCollectionContext> finishHandler = mFinishHandler;
    mFinishHandler = null;
    if (finishHandler != null) {
      finishHandler.accept(this);
    }
    for (LynxGlobalMemoryUsageCallback callback : callbacks) {
      invokeCallbackSafely(callback, result);
    }
  }

  static void invokeCallbackSafely(@NonNull LynxGlobalMemoryUsageCallback callback,
      @NonNull LynxGlobalMemoryUsageResult result) {
    try {
      callback.onResult(result);
    } catch (Throwable throwable) {
      // App callbacks are outside Lynx's control. Keep delivering the result to every coalesced
      // callback even if one host throws.
      LLog.e(TAG, "Failed to deliver Lynx memory usage result: " + throwable.getMessage());
    }
  }

  int getExpectedInstanceCount() {
    return mExpectedInstanceCount;
  }

  int getReceivedFetchResultCount() {
    return mReceivedFetchResultCount;
  }

  int getCallbackCount() {
    return mCallbacks.size();
  }
}
