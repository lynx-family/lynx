// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing.area;

import androidx.annotation.RestrictTo;
import com.lynx.tasm.performance.fsptracing.FSPConfig;

/**
 * Configuration for FSP Area tracing mode.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class FSPAreaConfig extends FSPConfig {
  // The threshold of minimum-milisecond interval between two snapshots.
  //
  // If the value is zero or negative, the tracer will not check for
  // interval.
  private double minimumDiffIntervalMs = 1.0;

  // The threshold of maximum acceptable area(in square pixels) differences
  // per-millisecond. Any content changes exceeding this threshold will delay the
  // FSP calculation.
  private double acceptableDifferencePerMs = 500.0;

  // The threshold of maximum acceptable area(in square pixels) differences per
  // snapshot. Any content changes exceeding this threshold will delay the FSP
  // calculation.
  private double acceptableDifferencePerSnapshot = 500.0;

  // The threshold of minimum acceptable area completion rate
  // The FSP tracer will wait until the completion rate of the
  // area projection is greater than this value before marking the timing
  // of FSP.
  //
  // If the value is zero or negative, the tracer will not check for
  // completion rate.
  private double minimumCompletionRate = 0.0;

  public double getMinimumDiffIntervalMs() {
    return minimumDiffIntervalMs;
  }

  public FSPAreaConfig setMinimumDiffIntervalMs(double value) {
    this.minimumDiffIntervalMs = Math.max(0.0, Math.min(value, 10000.0));
    return this;
  }

  public double getAcceptableDifferencePerMs() {
    return acceptableDifferencePerMs;
  }

  public FSPAreaConfig setAcceptableDifferencePerMs(double value) {
    this.acceptableDifferencePerMs = Math.max(1.0, Math.min(value, 10000.0));
    return this;
  }

  public double getAcceptableDifferencePerSnapshot() {
    return acceptableDifferencePerSnapshot;
  }

  public FSPAreaConfig setAcceptableDifferencePerSnapshot(double value) {
    this.acceptableDifferencePerSnapshot = Math.max(1.0, Math.min(value, 10000.0));
    return this;
  }

  public double getMinimumCompletionRate() {
    return minimumCompletionRate;
  }

  public FSPAreaConfig setMinimumCompletionRate(double value) {
    this.minimumCompletionRate = Math.max(0.0, Math.min(value, 1.0));
    return this;
  }
}
