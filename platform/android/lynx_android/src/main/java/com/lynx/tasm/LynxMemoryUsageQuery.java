// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.Collections;

/**
 * Public entry for querying current global Lynx memory usage across live Lynx instances.
 */
public final class LynxMemoryUsageQuery {
  private LynxMemoryUsageQuery() {}

  private static class Holder {
    private static final LynxMemoryUsageQuery INSTANCE = new LynxMemoryUsageQuery();
  }

  public static LynxMemoryUsageQuery inst() {
    return Holder.INSTANCE;
  }

  /**
   * Queries current global Lynx memory usage asynchronously.
   *
   * <p>The Android API is landed before the native collector implementation. Until the
   * collector-backed implementation is wired in, this method still preserves the asynchronous API
   * contract and invokes {@link LynxGlobalMemoryUsageCallback#onResult} with a non-null empty
   * {@link LynxMemoryCollectionStatus#COMPLETED} result. All byte/count fields are zero and
   * {@link LynxGlobalMemoryUsageResult#getInstances()} is empty in this placeholder response.
   *
   * <p>Threading: the callback may be invoked on any thread. Callers must dispatch to the main
   * thread before touching platform View objects.
   */
  @AnyThread
  public void queryLynxGlobalMemoryUsageAsync(@Nullable LynxGlobalMemoryUsageCallback callback) {
    if (callback == null) {
      return;
    }
    long collectionStartMs = System.currentTimeMillis();
    UIThreadUtils.runOnUiThread(
        () -> callback.onResult(createEmptyCompletedResult(collectionStartMs)));
  }

  @NonNull
  private static LynxGlobalMemoryUsageResult createEmptyCompletedResult(long collectionStartMs) {
    // Keep the public asynchronous contract active while this API-only facade is reviewed
    // independently from the native collector. The follow-up collector implementation will replace
    // this zero-valued placeholder with real global and per-instance memory attribution.
    return new LynxGlobalMemoryUsageResult(collectionStartMs, LynxMemoryCollectionStatus.COMPLETED,
        0L, 0L, 0, 0, 0L, 0L, 0D, 0L, 0L, 0L, 0L, 0L, Collections.emptyList());
  }
}
