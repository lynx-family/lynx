// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance;

import androidx.annotation.NonNull;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;

public interface IPerformanceObserver {
  /**
   * Notify the client that a performance event has been sent. It will be called every time a
   * performance event is generated, including but not limited to container initialization, engine
   * rendering, rendering metrics update, etc.
   *
   * Note: This method is for performance events and will be executed on the reporter thread, so do
   * not execute complex logic or UI modification logic in this method.
   *
   * @param entry the PerformanceEntry about the performance event
   *
   */
  public void onPerformanceEvent(@NonNull PerformanceEntry entry);
}
