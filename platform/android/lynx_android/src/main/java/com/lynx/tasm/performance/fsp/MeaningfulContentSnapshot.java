// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

import com.lynx.tasm.behavior.ui.MeaningfulPaintingArea;
import java.util.List;

/// MeaningfulContentSnapshot is a snapshot of meaningful painting areas.
public class MeaningfulContentSnapshot {
  private List<MeaningfulPaintingArea> mMeaningfulPaintingAreas = null;
  private int mContainerWidth = 0;
  private int mContainerHeight = 0;

  public MeaningfulContentSnapshot(int containerWidth, int containerHeight,
      List<MeaningfulPaintingArea> meaningfulPaintingAreas) {
    mContainerWidth = containerWidth;
    mContainerHeight = containerHeight;
    mMeaningfulPaintingAreas = meaningfulPaintingAreas;
  }

  public int getContainerWidth() {
    return mContainerWidth;
  }

  public int getContainerHeight() {
    return mContainerHeight;
  }

  public List<MeaningfulPaintingArea> getMeaningfulPaintingAreas() {
    return mMeaningfulPaintingAreas;
  }
}
