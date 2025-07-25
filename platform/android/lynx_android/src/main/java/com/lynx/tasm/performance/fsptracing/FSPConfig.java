// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing;

import androidx.annotation.RestrictTo;
import java.lang.Cloneable;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public class FSPConfig implements Cloneable {
  private double snapshotInterval = 0.0167;
  private double hardTimeout = 10.0;
  private double gracefulTimeout = 0.5;
  private double extendedGracefulTimeout = 1.0;

  public double getSnapshotInterval() {
    return snapshotInterval;
  }

  // Interval length of a snapshot in seconds.
  // The tracer will take snapshots of the page content status at this interval.
  public FSPConfig setSnapshotInterval(double value) {
    this.snapshotInterval = value;
    return this;
  }

  public double getHardTimeout() {
    return hardTimeout;
  }

  // Hard timeout for the entire tracing process
  // The tracer won't wait longer than this value to complete the FMP calculation.
  public FSPConfig setHardTimeout(double value) {
    this.hardTimeout = value;
    return this;
  }

  public double getGracefulTimeout() {
    return gracefulTimeout;
  }

  // Graceful timeout to wait when fmp has been reached
  //
  // This is useful for cases where the FMP is reached initially but the page
  // is still loading and any changes happening in the grace perdiod can also
  // be captured.
  public FSPConfig setGracefulTimeout(double value) {
    this.gracefulTimeout = value;
    return this;
  }

  public double getExtendedGracefulTimeout() {
    return extendedGracefulTimeout;
  }

  // Extended graceful timeout to wait when fmp has been reached and the page is
  // still loading.
  //
  // This is useful for cases where the FMP is reached initially but the page
  // is still loading and any changes happening in the grace perdiod can also
  // be captured.
  //
  // This timeout is used to extend the graceful timeout when there is at least
  // one ui elemnt that is still loading in the tree.
  public FSPConfig setExtendedGracefulTimeout(double value) {
    this.extendedGracefulTimeout = value;
    return this;
  }

  @Override
  public FSPConfig clone() {
    try {
      return (FSPConfig) super.clone();
    } catch (CloneNotSupportedException e) {
      return new FSPConfig();
    }
  }
}
