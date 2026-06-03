// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import com.lynx.tasm.base.LynxConsumer;

/**
 * Internal hook implemented by one live Lynx instance to contribute its memory snapshot.
 */
abstract class LynxMemoryUsageFetcher {
  /**
   * Starts one asynchronous per-instance query.
   *
   * <p>The callback uses {@link LynxConsumer} instead of {@code java.util.function.Consumer}
   * because Lynx still supports Android API levels below 24. Treat it as a simple one-argument
   * result callback: call {@link LynxConsumer#accept(Object)} with a snapshot when data is
   * available, or {@code null} when this instance cannot provide one. The collector wraps the
   * callback as single-shot, but implementations should still call it at most once to avoid wasting
   * work.
   */
  abstract void queryMemoryUsageAsync(@NonNull LynxConsumer<LynxInstanceMemoryUsage> callback);
}
