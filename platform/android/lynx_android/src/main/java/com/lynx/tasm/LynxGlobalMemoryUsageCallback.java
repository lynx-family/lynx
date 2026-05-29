// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;

/**
 * Callback for {@link LynxMemoryUsageQuery#queryLynxGlobalMemoryUsageAsync}.
 */
public abstract class LynxGlobalMemoryUsageCallback {
  /**
   * Called when a global memory usage result is ready.
   *
   * <p>The result is always non-null. Missing or not-yet-implemented data is represented by
   * {@link LynxMemoryCollectionStatus} and zero-valued or empty payload fields.
   *
   * <p>Threading: this method may be called on any thread. Dispatch to the main thread before
   * touching platform View objects.
   */
  @AnyThread
  public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {}
}
