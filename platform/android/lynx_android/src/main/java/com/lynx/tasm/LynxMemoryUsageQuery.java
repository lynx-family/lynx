// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.AnyThread;
import androidx.annotation.Keep;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.core.LynxThreadPool;
import java.util.Collections;

/**
 * Public entry for querying current global Lynx memory usage across live Lynx instances.
 */
public final class LynxMemoryUsageQuery {
  private static final String TAG = "LynxMemoryUsageQuery";
  private static final long DEFAULT_TIMEOUT_MS = 2000L;
  private static final LynxMemoryUsageQuery INSTANCE = new LynxMemoryUsageQuery();

  private LynxMemoryUsageQuery() {}

  public static LynxMemoryUsageQuery inst() {
    return INSTANCE;
  }

  /**
   * Queries current global Lynx memory usage asynchronously.
   *
   * <p>The callback runs on the Lynx report thread after native initialization. Before that, it
   * runs asynchronously on a background executor with an empty completed result.
   */
  @AnyThread
  public void queryLynxGlobalMemoryUsageAsync(@Nullable LynxGlobalMemoryUsageCallback callback) {
    queryLynxGlobalMemoryUsageAsync(callback, DEFAULT_TIMEOUT_MS);
  }

  /**
   * Queries current global Lynx memory usage asynchronously with a retained timeout metadata value.
   *
   * <p>The native global snapshot no longer fans out to per-instance fetchers, so it does not wait
   * for this timeout. Values less than or equal to zero use 2000ms for API compatibility.
   */
  @AnyThread
  public void queryLynxGlobalMemoryUsageAsync(
      @Nullable LynxGlobalMemoryUsageCallback callback, long timeoutMs) {
    if (callback == null) {
      return;
    }
    final long collectionStartMs = System.currentTimeMillis();
    final long collectionTimeoutMs = timeoutMs > 0 ? timeoutMs : DEFAULT_TIMEOUT_MS;
    if (!LynxEnv.inst().isNativeLibraryLoaded()) {
      LynxThreadPool.getAsyncServiceExecutor().execute(
          ()
              -> invokeCallbackSafely(callback,
                  LynxGlobalMemoryUsageResult.build(collectionStartMs,
                      LynxMemoryCollectionStatus.COMPLETED,
                      System.currentTimeMillis() - collectionStartMs, collectionTimeoutMs, 0, 0L,
                      Collections.emptyList())));
      return;
    }
    nativeQueryGlobalMemoryUsageAsync(callback, collectionStartMs, collectionTimeoutMs);
  }

  @Keep
  @CalledByNative
  private static void onNativeMemoryUsageResult(LynxGlobalMemoryUsageCallback callback,
      long collectionStartMs, long collectionTimeoutMs, long[] globalValues, long[] instanceValues,
      String[] instanceStrings) {
    invokeCallbackSafely(callback,
        LynxGlobalMemoryUsageResult.fromNative(collectionStartMs,
            System.currentTimeMillis() - collectionStartMs, collectionTimeoutMs, globalValues,
            instanceValues, instanceStrings));
  }

  private static void invokeCallbackSafely(
      LynxGlobalMemoryUsageCallback callback, LynxGlobalMemoryUsageResult result) {
    try {
      callback.onResult(result);
    } catch (Throwable throwable) {
      LLog.e(TAG, "Failed to deliver memory usage result: " + throwable.getMessage());
    }
  }

  private static native void nativeQueryGlobalMemoryUsageAsync(
      LynxGlobalMemoryUsageCallback callback, long collectionStartMs, long collectionTimeoutMs);
}
