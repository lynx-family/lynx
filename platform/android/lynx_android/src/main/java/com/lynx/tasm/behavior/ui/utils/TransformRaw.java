// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.utils;

import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_MATRIX;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_MATRIX_3d;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_3d;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_X;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_Y;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_Z;

import android.opengl.Matrix;
import android.renderscript.Matrix4f;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.StyleConstants;
import java.util.ArrayList;
import java.util.List;

public class TransformRaw {
  /**
   * transform type
   */
  private final int transformType;
  /**
   * param 1
   */
  private final float p0;
  private final PlatformLength platformLengthP0;
  private final int p0Unit;
  /**
   * param 2
   */
  private final float p1;
  private final PlatformLength platformLengthP1;
  private final int p1Unit;
  /**
   * param 3
   */
  private final float p2;
  private final PlatformLength platformLengthP2;
  private final int p2Unit;

  private final float[] transformRawData;

  private TransformRaw(
      int transformType, float p0, int p0Unit, float p1, int p1Unit, float p2, int p2Unit) {
    this.transformType = transformType;

    this.platformLengthP0 = new PlatformLength(0, 0);
    this.p0 = p0;
    this.p0Unit = p0Unit;

    this.platformLengthP1 = new PlatformLength(0, 0);
    this.p1 = p1;
    this.p1Unit = p1Unit;

    this.platformLengthP2 = new PlatformLength(0, 0);
    this.p2 = p2;
    this.p2Unit = p2Unit;

    transformRawData = new Matrix4f().getArray();
  }

  private TransformRaw(int transformType, PlatformLength platformLengthP0, int p0Unit,
      PlatformLength platformLengthP1, int p1Unit, PlatformLength platformLengthP2, int p2Unit) {
    this.transformType = transformType;

    this.platformLengthP0 = platformLengthP0;
    this.p0 = 0;
    this.p0Unit = p0Unit;

    this.platformLengthP1 = platformLengthP1;
    this.p1 = 0;
    this.p1Unit = p1Unit;

    this.platformLengthP2 = platformLengthP2;
    this.p2 = 0;
    this.p2Unit = p2Unit;

    transformRawData = new Matrix4f().getArray();
  }

  private TransformRaw(int transformType, float[] transform_raw_data) {
    this.transformType = transformType;

    this.platformLengthP0 = new PlatformLength(0, 0);
    this.p0 = 0;
    this.p0Unit = 0;

    this.platformLengthP1 = new PlatformLength(0, 0);
    this.p1 = 0;
    this.p1Unit = 0;

    this.platformLengthP2 = new PlatformLength(0, 0);
    this.p2 = 0;
    this.p2Unit = 0;

    this.transformRawData = transform_raw_data;
  }

  public int getTransformType() {
    return transformType;
  }

  public float getP0() {
    return p0;
  }

  public PlatformLength getPlatformLengthP0() {
    return platformLengthP0;
  }

  public boolean isP0Percent() {
    return p0Unit == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT;
  }

  public float getP1() {
    return p1;
  }

  public PlatformLength getPlatformLengthP1() {
    return platformLengthP1;
  }

  public boolean isP1Percent() {
    return p1Unit == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT;
  }

  public float getP2() {
    return p2;
  }

  public PlatformLength getPlatformLengthP2() {
    return platformLengthP2;
  }

  public boolean isP2Percent() {
    return p2Unit == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT;
  }

  public boolean hasPercent() {
    return isP0Percent() || isP1Percent() || isP2Percent();
  }

  public float[] getTransformRawData() {
    return transformRawData;
  }

  @Nullable
  public static List<TransformRaw> toTransformRaw(ReadableArray items) {
    if (null == items || items.size() <= 0) {
      return null;
    }
    List<TransformRaw> result = new ArrayList<>();
    // TODO(liyanbo): this will replace by drawInfo. this is temp solution.
    // details can @see ComputedPlatformCSSStyle::TransformToLepus()
    for (int i = 0; i < items.size(); i++) {
      ReadableArray ra = items.getArray(i);
      if (ra.size() < 7) {
        LLog.DTHROW(new IllegalArgumentException("transform params is error."));
        continue;
      }
      int transType = ra.getInt(0);

      if (transType == TRANSFORM_MATRIX || transType == TRANSFORM_MATRIX_3d) {
        float[] transform_raw_data = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f};
        for (int pos = 0; pos < 4 * 4; ++pos) {
          transform_raw_data[pos] = (float) ra.getDouble(pos + 1);
        }
        result.add(new TransformRaw(transType, transform_raw_data));
      } else {
        int p0Unit = ra.getInt(2);
        int p1Unit = ra.getInt(4);
        int p2Unit = ra.getInt(6);
        if (transType == TRANSFORM_TRANSLATE || transType == TRANSFORM_TRANSLATE_X
            || transType == TRANSFORM_TRANSLATE_Y || transType == TRANSFORM_TRANSLATE_Z
            || transType == TRANSFORM_TRANSLATE_3d) {
          PlatformLength platformLengthP0 = new PlatformLength(ra.getDynamic(1), p0Unit);
          PlatformLength platformLengthP1 = new PlatformLength(ra.getDynamic(3), p1Unit);
          PlatformLength platformLengthP2 = new PlatformLength(ra.getDynamic(5), p2Unit);
          result.add(new TransformRaw(transType, platformLengthP0, p0Unit, platformLengthP1, p1Unit,
              platformLengthP2, p2Unit));
        } else {
          float p0 = (float) ra.getDouble(1);
          float p1 = (float) ra.getDouble(3);
          float p2 = (float) ra.getDouble(5);
          result.add(new TransformRaw(transType, p0, p0Unit, p1, p1Unit, p2, p2Unit));
        }
      }
    }

    return result;
  }
  public static boolean hasPercent(List<TransformRaw> transforms) {
    if (null == transforms || transforms.isEmpty()) {
      return false;
    }
    for (TransformRaw tr : transforms) {
      if (tr.hasPercent()) {
        return true;
      }
    }
    return false;
  }

  public static float hasZValue(List<TransformRaw> transforms) {
    if (null == transforms || transforms.isEmpty()) {
      return 0;
    }

    float ret = .0f;

    for (TransformRaw tr : transforms) {
      switch (tr.transformType) {
        case TRANSFORM_TRANSLATE_Z:
          // translateZ(zValue)
          ret = tr.platformLengthP0.asNumber();
          break;
        case TRANSFORM_TRANSLATE_3d:
          // translate3D(x,y,z)
          ret = tr.platformLengthP2.asNumber();
          break;
      }
    }

    return ret;
  }

  public static float hasXValue(List<TransformRaw> transforms) {
    if (null == transforms || transforms.isEmpty()) {
      return 0;
    }

    float ret = .0f;

    for (TransformRaw tr : transforms) {
      switch (tr.transformType) {
        case TRANSFORM_TRANSLATE_X:
        case TRANSFORM_TRANSLATE_3d:
          // translateX(xValue)
          // translate(x,y)
          ret = tr.platformLengthP0.asNumber();
          break;
      }
    }

    return ret;
  }

  public static float hasYValue(List<TransformRaw> transforms) {
    if (null == transforms || transforms.isEmpty()) {
      return 0;
    }

    float ret = .0f;

    for (TransformRaw tr : transforms) {
      switch (tr.transformType) {
        case TRANSFORM_TRANSLATE_Y:
          // translateY(xValue)
          ret = tr.platformLengthP0.asNumber();
          break;
        case TRANSFORM_TRANSLATE_3d:
          // translate(x,y)
          ret = tr.platformLengthP1.asNumber();
          break;
      }
    }

    return ret;
  }
}
