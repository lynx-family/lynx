// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

public class MeasureResult {
  private static final int RESULT_WIDTH = 0, RESULT_HEIGHT = 1, RESULT_BASELINE = 2;
  private static final int NUM_RESULT = 3;
  private float[] measureResult = new float[NUM_RESULT];

  public MeasureResult(float widthResult, float heightResult) {
    measureResult[RESULT_WIDTH] = widthResult;
    measureResult[RESULT_HEIGHT] = heightResult;
    measureResult[RESULT_BASELINE] = 0;
  }

  public MeasureResult(float widthResult, float heightResult, float baselineResult) {
    measureResult[RESULT_WIDTH] = widthResult;
    measureResult[RESULT_HEIGHT] = heightResult;
    measureResult[RESULT_BASELINE] = baselineResult;
  }

  public float getWidthResult() {
    return measureResult[RESULT_WIDTH];
  }

  public float getHeightResult() {
    return measureResult[RESULT_HEIGHT];
  }

  public float getBaselineResult() {
    return measureResult[RESULT_BASELINE];
  }
}
