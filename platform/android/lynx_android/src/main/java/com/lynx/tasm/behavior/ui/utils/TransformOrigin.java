// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.utils;

import static androidx.annotation.RestrictTo.Scope.LIBRARY_GROUP;

import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.behavior.StyleConstants;

public class TransformOrigin {
  /**
   * first position
   */
  private final float p0;
  private final int p0Unit;

  /**
   * second position
   */
  private final float p1;
  private final int p1Unit;

  @RestrictTo(LIBRARY_GROUP)
  @Nullable
  public static TransformOrigin MakeTransformOrigin(ReadableArray array) {
    if (null == array || array.size() < 2) {
      return null;
    }
    return new TransformOrigin(array);
  }
  @RestrictTo(LIBRARY_GROUP)
  public static final TransformOrigin TRANSFORM_ORIGIN_DEFAULT = new TransformOrigin();

  private TransformOrigin() {
    this.p0 = 0.5f;
    this.p0Unit = StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT;
    this.p1 = 0.5f;
    this.p1Unit = StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT;
  }

  private TransformOrigin(ReadableArray array) {
    this.p0 = (float) array.getDouble(0);
    this.p0Unit = array.getInt(1);
    if (array.size() >= 4) {
      this.p1 = (float) array.getDouble(2);
      this.p1Unit = array.getInt(3);
    } else {
      p1 = 0.5f;
      p1Unit = StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT;
    }
  }

  private TransformOrigin(float originX, float originY) {
    this.p0 = originX;
    this.p1 = originY;
    this.p0Unit = 0;
    this.p1Unit = 0;
  }

  public boolean isValid() {
    return isP0Valid() || isP1Valid();
  }

  public boolean isP0Valid() {
    return !(p0 == 0.5f && p0Unit == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT);
  }
  public boolean isP1Valid() {
    return !(p1 == 0.5f && p1Unit == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT);
  }

  public boolean isP0Percent() {
    return p0Unit == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT;
  }

  public boolean isP1Percent() {
    return p1Unit == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT;
  }

  public boolean hasPercent() {
    return isP0Percent() || isP1Percent();
  }

  public float getP0() {
    return p0;
  }

  public float getP1() {
    return p1;
  }

  public static boolean hasPercent(@Nullable TransformOrigin to) {
    return null != to && to.isValid() && to.hasPercent();
  }
}
