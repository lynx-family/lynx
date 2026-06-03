// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.AnyThread;
import androidx.annotation.Nullable;

/**
 * Public entry for querying current global Lynx memory usage across live Lynx instances.
 */
public final class LynxMemoryUsageQuery {
  private static final LynxMemoryUsageQuery INSTANCE = new LynxMemoryUsageQuery();

  private LynxMemoryUsageQuery() {}

  public static LynxMemoryUsageQuery inst() {
    return INSTANCE;
  }

  /**
   * Queries current global Lynx memory usage asynchronously.
   *
   * <p>Threading: when the Lynx native library is available, the callback is invoked on the Lynx
   * report thread. Before the native report thread is available, the callback is invoked
   * asynchronously on a background executor with an empty completed result. Callers must dispatch
   * to the main thread before touching platform View objects.
   */
  @AnyThread
  public void queryLynxGlobalMemoryUsageAsync(@Nullable LynxGlobalMemoryUsageCallback callback) {
    LynxGlobalMemoryUsageCollector.getInstance().queryMemoryUsageAsync(callback);
  }

  /**
   * Queries current global Lynx memory usage asynchronously with a custom collection timeout.
   *
   * <p>When {@code timeoutMs <= 0}, the collector uses the default timeout of 2000ms.
   *
   * <p>Threading: when the Lynx native library is available, the callback is invoked on the Lynx
   * report thread. Before the native report thread is available, the callback is invoked
   * asynchronously on a background executor with an empty completed result. Callers must dispatch
   * to the main thread before touching platform View objects.
   */
  @AnyThread
  public void queryLynxGlobalMemoryUsageAsync(
      @Nullable LynxGlobalMemoryUsageCallback callback, long timeoutMs) {
    LynxGlobalMemoryUsageCollector.getInstance().queryMemoryUsageAsync(callback, timeoutMs);
  }
}
