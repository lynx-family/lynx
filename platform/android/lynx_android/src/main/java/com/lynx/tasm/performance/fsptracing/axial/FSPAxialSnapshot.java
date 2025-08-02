// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing.axial;

import androidx.annotation.RestrictTo;
import com.lynx.tasm.performance.fsptracing.FSPSnapshot;
import java.util.BitSet;

/**
 * Snapshot implementation for axial FSP tracing.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class FSPAxialSnapshot extends FSPSnapshot {
  public static final int X_PROJECTIONS_LEN = 256;
  public static final int Y_PROJECTIONS_LEN = 512;

  private final BitSet mXProjections = new BitSet(X_PROJECTIONS_LEN);
  private final BitSet mYProjections = new BitSet(Y_PROJECTIONS_LEN);
  private int mProjectionWidth;
  private int mProjectionHeight;

  public FSPAxialSnapshot() {}

  public BitSet getXProjections() {
    return mXProjections;
  }

  public BitSet getYProjections() {
    return mYProjections;
  }

  public int getProjectionWidth() {
    return mProjectionWidth;
  }

  public int getProjectionHeight() {
    return mProjectionHeight;
  }

  public void setProjectionWidth(int projectionWidth) {
    mProjectionWidth = projectionWidth;
  }

  public void setProjectionHeight(int projectionHeight) {
    mProjectionHeight = projectionHeight;
  }

  @Override
  protected void resetImpl() {
    mXProjections.clear();
    mYProjections.clear();
    mProjectionWidth = 0;
    mProjectionHeight = 0;
  }
}
