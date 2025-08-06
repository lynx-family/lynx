// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing.area;

import androidx.annotation.RestrictTo;
import com.lynx.tasm.performance.fsptracing.FSPSnapshot;
import java.util.BitSet;

/**
 * Snapshot implementation for area FSP tracing.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class FSPAreaSnapshot extends FSPSnapshot {
  public static final int X_PROJECTIONS_LEN = 256;
  public static final int Y_PROJECTIONS_LEN = 512;
  public static final int PROJECTIONS_LEN = X_PROJECTIONS_LEN * Y_PROJECTIONS_LEN;

  private final BitSet mProjections = new BitSet(PROJECTIONS_LEN);
  private int mProjectionArea;

  public FSPAreaSnapshot() {}

  public BitSet getProjections() {
    return mProjections;
  }

  public int getProjectionArea() {
    return mProjectionArea;
  }

  public void setProjectionArea(int projectionArea) {
    mProjectionArea = projectionArea;
  }

  @Override
  protected void resetImpl() {
    mProjections.clear();
    mProjectionArea = 0;
  }
}
