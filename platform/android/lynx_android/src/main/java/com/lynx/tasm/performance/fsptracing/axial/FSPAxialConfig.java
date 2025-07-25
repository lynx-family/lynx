// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing.axial;

import androidx.annotation.RestrictTo;
import com.lynx.tasm.performance.fsptracing.FSPConfig;

/**
 * Configuration for FSP Axial tracing mode.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class FSPAxialConfig extends FSPConfig {
  // The threshold of minimum-milisecond interval between two snapshots.
  //
  // If the value is zero or negative, the tracer will not check for
  // interval.
  private double minimumDiffIntervalMs = 1.0;
  
  // The threshold of maximum acceptable pixel differences per-millisecond.
  // Any content changes exceeding this threshold will delay the FSP
  // calculation.
  private double acceptablePixelDifferencePerMs = 2.0;

  // The threshold of maximum acceptable pixel differences per snapshot.
  // Any content changes exceeding this threshold will delay the FSP
  // calculation.
  private double acceptablePixelDifferencePerSnapshot = 50.0;

  // The threshold of minimum acceptable pixel projection completion rate in x
  // axis. The FSP tracer will wait until the completion rate of the
  // pixel projection is greater than this value before marking the timing
  // of FSP.
  //
  // If the value is zero or negative, the tracer will not check for
  // completion rate.
  private double minimumCompletionRateX = 0.0;

  // The threshold of minimum acceptable pixel projection completion rate in y
  // axis. The FSP tracer will wait until the completion rate of the
  // pixel projection is greater than this value before marking the timing
  // of FSP.
  //
  // If the value is zero or negative, the tracer will not check for
  // completion rate.
  private double minimumCompletionRateY = 0.0;

  public double getMinimumDiffIntervalMs() {
    return minimumDiffIntervalMs;
  }

  public FSPAxialConfig setMinimumDiffIntervalMs(double value) {
    this.minimumDiffIntervalMs = Math.max(0.0, Math.min(value, 10000.0));
    return this;
  }

  public double getAcceptablePixelDifferencePerMs() {
    return acceptablePixelDifferencePerMs;
  }

  public FSPAxialConfig setAcceptablePixelDifferencePerMs(double value) {
    this.acceptablePixelDifferencePerMs = Math.max(1.0, Math.min(value, 10000.0));
    return this;
  }

  public double getAcceptablePixelDifferencePerSnapshot() {
    return acceptablePixelDifferencePerSnapshot;
  }

  public FSPAxialConfig setAcceptablePixelDifferencePerSnapshot(double value) {
    this.acceptablePixelDifferencePerSnapshot = Math.max(1.0, Math.min(value, 10000.0));
    return this;
  }

  public double getMinimumCompletionRateX() {
    return minimumCompletionRateX;
  }

  public FSPAxialConfig setMinimumCompletionRateX(double value) {
    this.minimumCompletionRateX = Math.max(0.0, Math.min(value, 1.0));
    return this;
  }

  public double getMinimumCompletionRateY() {
    return minimumCompletionRateY;
  }

  public FSPAxialConfig setMinimumCompletionRateY(double value) {
    this.minimumCompletionRateY = Math.max(0.0, Math.min(value, 1.0));
    return this;
  }
}
