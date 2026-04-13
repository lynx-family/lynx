// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.animation;

import static org.junit.Assert.assertEquals;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class InterpolatorFactoryTest {
  @Test
  public void cubicBezierWithOutOfRangeControlPointDoesNotCrash() {
    AnimationInfo info = new AnimationInfo();
    info.setTimingFunction(AnimationConstant.INTERCEPTOR_CUBIC_BEZIER, 1.2f, 0.4f, -0.2f, 0.9f, 0);

    assertEquals(0.5f, InterpolatorFactory.getInterpolator(info).getInterpolation(0.5f), 0.0f);
  }

  @Test
  public void squareBezierWithOutOfRangeControlPointDoesNotCrash() {
    AnimationInfo info = new AnimationInfo();
    info.setTimingFunction(AnimationConstant.INTERCEPTOR_SQUARE_BEZIER, 1.4f, 0.5f, 0.0f, 0.0f, 0);

    assertEquals(0.5f, InterpolatorFactory.getInterpolator(info).getInterpolation(0.5f), 0.0f);
  }

  @Test
  public void cubicBezierWithNaNFallsBackToLinear() {
    AnimationInfo info = new AnimationInfo();
    info.setTimingFunction(
        AnimationConstant.INTERCEPTOR_CUBIC_BEZIER, Float.NaN, 0.4f, 0.2f, 0.9f, 0);

    assertEquals(0.5f, InterpolatorFactory.getInterpolator(info).getInterpolation(0.5f), 0.0f);
  }
}
