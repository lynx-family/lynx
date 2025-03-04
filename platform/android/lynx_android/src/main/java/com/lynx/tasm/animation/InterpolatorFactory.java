// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.animation;

import static com.lynx.tasm.animation.AnimationConstant.INTERCEPTOR_CUBIC_BEZIER;
import static com.lynx.tasm.animation.AnimationConstant.INTERCEPTOR_EASE_IN;
import static com.lynx.tasm.animation.AnimationConstant.INTERCEPTOR_EASE_IN_OUT;
import static com.lynx.tasm.animation.AnimationConstant.INTERCEPTOR_EASE_OUT;
import static com.lynx.tasm.animation.AnimationConstant.INTERCEPTOR_LINEAR;
import static com.lynx.tasm.animation.AnimationConstant.INTERCEPTOR_SQUARE_BEZIER;
import static com.lynx.tasm.animation.AnimationConstant.INTERCEPTOR_STEPS;

import android.util.SparseArray;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.BaseInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Interpolator;
import android.view.animation.LinearInterpolator;
import androidx.core.view.animation.PathInterpolatorCompat;
import com.lynx.tasm.base.LLog;

public class InterpolatorFactory {
  private static final SparseArray<BaseInterpolator> INTERPOLATOR =
      new SparseArray<BaseInterpolator>() {
        {
          put(INTERCEPTOR_LINEAR, new LinearInterpolator());
          put(INTERCEPTOR_EASE_IN, new AccelerateInterpolator());
          put(INTERCEPTOR_EASE_OUT, new DecelerateInterpolator());
          put(INTERCEPTOR_EASE_IN_OUT, new AccelerateDecelerateInterpolator());
        }
      };
  public static Interpolator getInterpolator(AnimationInfo ai) {
    int timingType = ai.getTimingType();
    switch (timingType) {
      case INTERCEPTOR_LINEAR:
      case INTERCEPTOR_EASE_IN:
      case INTERCEPTOR_EASE_OUT:
      case INTERCEPTOR_EASE_IN_OUT:
        return INTERPOLATOR.get(timingType);
      case INTERCEPTOR_SQUARE_BEZIER:
        return PathInterpolatorCompat.create(ai.getX1(), ai.getY1());
      case INTERCEPTOR_CUBIC_BEZIER:
        return PathInterpolatorCompat.create(ai.getX1(), ai.getY1(), ai.getX2(), ai.getY2());
      case INTERCEPTOR_STEPS:
        return new StepsInterpolation(ai.getCount(), ai.getStepsType());
      default:
        LLog.DTHROW(
            new RuntimeException("layout animation don't support interpolator:" + timingType));
        return INTERPOLATOR.get(INTERCEPTOR_LINEAR);
    }
  }

  private static final int LynxAnimationStepsJumpStart = 1;
  private static final int LynxAnimationStepsJumpEnd = 2;
  private static final int LynxAnimationStepsJumpNone = 3;
  private static final int LynxAnimationStepsJumpBoth = 4;
  private static class StepsInterpolation implements Interpolator {
    private int mCount;
    private int mJump;

    StepsInterpolation(int count, int jump) {
      mCount = count;
      mJump = jump;
    }

    @Override
    public float getInterpolation(float input) {
      int state;
      switch (mJump) {
        case LynxAnimationStepsJumpStart:
          state = (int) (input * mCount) + 1;
          if (state > mCount)
            state = mCount;
          return ((float) state) / mCount;
        case LynxAnimationStepsJumpEnd:
          state = (int) (input * mCount);
          if (state == mCount)
            state -= 1;
          return ((float) state) / mCount;
        case LynxAnimationStepsJumpBoth:
          state = (int) (input * mCount) + 1;
          if (state > mCount)
            state = mCount;
          return ((float) state) / (mCount + 1);
        case LynxAnimationStepsJumpNone:
          state = (int) (input * mCount);
          if (state == mCount)
            state -= 1;
          return ((float) state) / (mCount - 1);
        default:
          return 0;
      }
    }
  }
}
