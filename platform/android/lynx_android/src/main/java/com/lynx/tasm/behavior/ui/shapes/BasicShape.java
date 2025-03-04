// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.shapes;

import android.graphics.Matrix;
import android.graphics.Path;
import android.graphics.RectF;
import androidx.annotation.Nullable;
import androidx.core.graphics.PathParser;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.ui.utils.BorderRadius;

public class BasicShape {
  public static BasicShape CreateFromReadableArray(
      @Nullable ReadableArray paramsArray, float scaledDensity) {
    BasicShape basicShape = null;
    if (paramsArray == null || paramsArray.size() <= 1) {
      // Native parse error or reset.
      // Array is empty when reset.
      return null;
    }
    int len = paramsArray.size();
    long type = paramsArray.getLong(INDEX_BASIC_SHAPE_TYPE);
    if (type == StyleConstants.BASIC_SHAPE_TYPE_PATH) {
      if (len != RAW_PARAMS_LEN_PATH) {
        return null;
      }
      String data = paramsArray.getString(RAW_INDEX_PATH_DATA);
      basicShape = new BasicShape(data, scaledDensity);
    } else if (type == StyleConstants.BASIC_SHAPE_TYPE_SUPER_ELLIPSE) {
      if (len != RAW_PARAMS_LEN_SUPER_ELLIPSE) {
        return null;
      }
      // decompose array [rx, unit, ry, unit, ex, ey, cx, unit, cy, unit]
      // into params{rx, ry, cx, cy} and exponents{ex, ey}
      basicShape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_SUPER_ELLIPSE);
      basicShape.params = new Length[PARAMS_LEN_SUPER_ELLIPSE];
      basicShape.params[INDEX_SUPER_ELLIPSE_RADIUS_X] =
          new Length(paramsArray.getDouble(RAW_INDEX_SUPER_ELLIPSE_RADIUS_X),
              paramsArray.getInt(RAW_INDEX_SUPER_ELLIPSE_RADIUS_X_UNIT));
      basicShape.params[INDEX_SUPER_ELLIPSE_RADIUS_Y] =
          new Length(paramsArray.getDouble(RAW_INDEX_SUPER_ELLIPSE_RADIUS_Y),
              paramsArray.getInt(RAW_INDEX_SUPER_ELLIPSE_RADIUS_Y_UNIT));
      basicShape.params[INDEX_SUPER_ELLIPSE_CENTER_X] =
          new Length(paramsArray.getDouble(RAW_INDEX_SUPER_ELLIPSE_CENTER_X),
              paramsArray.getInt(RAW_INDEX_SUPER_ELLIPSE_CENTER_X_UNIT));
      basicShape.params[INDEX_SUPER_ELLIPSE_CENTER_Y] =
          new Length(paramsArray.getDouble(RAW_INDEX_SUPER_ELLIPSE_CENTER_Y),
              paramsArray.getInt(RAW_INDEX_SUPER_ELLIPSE_CENTER_Y_UNIT));

      basicShape.exponents = new double[PARAMS_LEN_SUPER_ELLIPSE_EXPONENTS];
      basicShape.exponents[INDEX_SUPER_ELLIPSE_EXPONENT_X] =
          paramsArray.getDouble(RAW_INDEX_SUPER_ELLIPSE_EXPONENT_X);
      basicShape.exponents[INDEX_SUPER_ELLIPSE_EXPONENT_Y] =
          paramsArray.getDouble(RAW_INDEX_SUPER_ELLIPSE_EXPONENT_Y);
    } else if (type == StyleConstants.BASIC_SHAPE_TYPE_CIRCLE) {
      if (len != RAW_PARAMS_LEN_CIRCLE) {
        return null;
      }
      // decompose array {r, unit, cx, unit, cy, unit} into params {r, cx, cy}
      basicShape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_CIRCLE);
      basicShape.params = new Length[PARAMS_LEN_CIRCLE];
      basicShape.params[INDEX_CIRCLE_RADIUS] =
          new Length(paramsArray.getDouble(RAW_INDEX_CIRCLE_RADIUS),
              paramsArray.getInt(RAW_INDEX_CIRCLE_RADIUS_UNIT));
      basicShape.params[INDEX_CIRCLE_CENTER_X] =
          new Length(paramsArray.getDouble(RAW_INDEX_CIRCLE_CENTER_X),
              paramsArray.getInt(RAW_INDEX_CIRCLE_CENTER_X_UNIT));
      basicShape.params[INDEX_CIRCLE_CENTER_Y] =
          new Length(paramsArray.getDouble(RAW_INDEX_CIRCLE_CENTER_Y),
              paramsArray.getInt(RAW_INDEX_CIRCLE_CENTER_Y_UNIT));

    } else if (type == StyleConstants.BASIC_SHAPE_TYPE_ELLIPSE) {
      if (len != RAW_PARAMS_LEN_ELLIPSE) {
        return null;
      }
      // decompose array {rx, unit, ry, unit, cx, unit, cy, unit}
      // into params {rx, ry, cx, cy}
      basicShape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_ELLIPSE);
      basicShape.params = new Length[PARAMS_LEN_ELLIPSE];
      basicShape.params[INDEX_ELLIPSE_RADIUS_X] =
          new Length(paramsArray.getDouble(RAW_INDEX_ELLIPSE_RADIUS_X),
              paramsArray.getInt(RAW_INDEX_ELLIPSE_RADIUS_X_UNIT));
      basicShape.params[INDEX_ELLIPSE_RADIUS_Y] =
          new Length(paramsArray.getDouble(RAW_INDEX_ELLIPSE_RADIUS_Y),
              paramsArray.getInt(RAW_INDEX_ELLIPSE_RADIUS_Y_UNIT));
      basicShape.params[INDEX_ELLIPSE_CENTER_X] =
          new Length(paramsArray.getDouble(RAW_INDEX_ELLIPSE_CENTER_X),
              paramsArray.getInt(RAW_INDEX_ELLIPSE_CENTER_X_UNIT));
      basicShape.params[INDEX_ELLIPSE_CENTER_Y] =
          new Length(paramsArray.getDouble(RAW_INDEX_ELLIPSE_CENTER_Y),
              paramsArray.getInt(RAW_INDEX_ELLIPSE_CENTER_Y_UNIT));
    } else if (type == StyleConstants.BASIC_SHAPE_TYPE_INSET) {
      // begin handling inset function params
      // clang-format off
      // params arrange as:
      // first 8 fields are inset:
      // | top | lengthUnit | right | lengthUnit | bottom | lengthUnit | left | lengthUnit |
      // clang-format on
      basicShape = new BasicShape(StyleConstants.BASIC_SHAPE_TYPE_INSET);
      basicShape.params = new Length[4];
      if (paramsArray.size() == RAW_PARAMS_LEN_INSET_RECT) {
        // Rect only has first 8 params.
        basicShape.mCornerType = CORNER_RECT;
      } else if (paramsArray.size() == RAW_PARAMS_LEN_INSET_ROUND) {
        // clang-format off
        // inset with rounded and lame corner has 16 radius fields:
        // |   top-left-x  | lengthUnit |   top-left-y   | lengthUnit |   top-right-x  | lengthUnit |
        // |  top-right-y  | lengthUnit | bottom-right-x | lengthUnit | bottom-right-y | lengthUnit |
        // | bottom-left-x | lengthUnit |  bottom-left-y | lengthUnit |
        // clang-format on
        basicShape.mCornerType = CORNER_ROUNDED;
      } else if (paramsArray.size() == RAW_PARAMS_LEN_INSET_SUPER_ELLIPSE) {
        // super-ellipse has two more fields for exponents before border-radius.
        // | exponent-x | exponent-y | border-radius ...|
        basicShape.mCornerType = CORNER_SUPER_ELLIPTICAL;
      } else {
        // invalid params array
        return null;
      }
      for (int i = 0; i < 4; i++) {
        basicShape.params[i] =
            new Length(paramsArray.getDouble(2 * i + 1), paramsArray.getInt(2 * i + 2));
      }
      // rounded corner's radius value start at index 9
      int radiusOffset = 9;
      switch (basicShape.mCornerType) {
        case CORNER_RECT:
          // rect corner don't have radius.
          break;
        case CORNER_SUPER_ELLIPTICAL:
          // super elliptical corner has two exponents, the radius value start at index 11.
          basicShape.exponents = new double[2];
          basicShape.exponents[INDEX_SUPER_ELLIPSE_EXPONENT_X] = paramsArray.getDouble(9);
          basicShape.exponents[INDEX_SUPER_ELLIPSE_EXPONENT_Y] = paramsArray.getDouble(10);
          radiusOffset = 11;
        case CORNER_ROUNDED:
          // append the corner radius values to params array.
          basicShape.mCornerRadius = new BorderRadius();

          for (int i = 0; i < 4; i++) {
            BorderRadius.Corner corner =
                BorderRadius.Corner.toCorner(paramsArray, 4 * i + radiusOffset);
            basicShape.mCornerRadius.setCorner(i, corner);
          }
      }
    } else {
      return null;
    }
    return basicShape;
  }
  private final static String BASIC_SHAPE_TAG = "LynxBasicShape";
  // Constant definitions for raw data array.
  public static final int RAW_PARAMS_LEN_PATH = 2;
  public static final int RAW_PARAMS_LEN_CIRCLE = 7;
  public static final int RAW_PARAMS_LEN_ELLIPSE = 9;
  public static final int RAW_PARAMS_LEN_SUPER_ELLIPSE = 11;
  public static final int RAW_INDEX_PATH_DATA = 1;
  public static final int RAW_INDEX_CIRCLE_RADIUS = 1;
  public static final int RAW_INDEX_CIRCLE_RADIUS_UNIT = 2;
  public static final int RAW_INDEX_CIRCLE_CENTER_X = 3;
  public static final int RAW_INDEX_CIRCLE_CENTER_X_UNIT = 4;
  public static final int RAW_INDEX_CIRCLE_CENTER_Y = 5;
  public static final int RAW_INDEX_CIRCLE_CENTER_Y_UNIT = 6;

  public static final int RAW_INDEX_ELLIPSE_RADIUS_X = 1;
  public static final int RAW_INDEX_ELLIPSE_RADIUS_X_UNIT = 2;
  public static final int RAW_INDEX_ELLIPSE_RADIUS_Y = 3;
  public static final int RAW_INDEX_ELLIPSE_RADIUS_Y_UNIT = 4;
  public static final int RAW_INDEX_ELLIPSE_CENTER_X = 5;
  public static final int RAW_INDEX_ELLIPSE_CENTER_X_UNIT = 6;
  public static final int RAW_INDEX_ELLIPSE_CENTER_Y = 7;
  public static final int RAW_INDEX_ELLIPSE_CENTER_Y_UNIT = 8;

  public static final int RAW_INDEX_SUPER_ELLIPSE_RADIUS_X = 1;
  public static final int RAW_INDEX_SUPER_ELLIPSE_RADIUS_X_UNIT = 2;
  public static final int RAW_INDEX_SUPER_ELLIPSE_RADIUS_Y = 3;
  public static final int RAW_INDEX_SUPER_ELLIPSE_RADIUS_Y_UNIT = 4;
  public static final int RAW_INDEX_SUPER_ELLIPSE_EXPONENT_X = 5;
  public static final int RAW_INDEX_SUPER_ELLIPSE_EXPONENT_Y = 6;
  public static final int RAW_INDEX_SUPER_ELLIPSE_CENTER_X = 7;
  public static final int RAW_INDEX_SUPER_ELLIPSE_CENTER_X_UNIT = 8;
  public static final int RAW_INDEX_SUPER_ELLIPSE_CENTER_Y = 9;
  public static final int RAW_INDEX_SUPER_ELLIPSE_CENTER_Y_UNIT = 10;

  // Constant definition for decomposed param array
  // Circle
  public static final int PARAMS_LEN_CIRCLE = 3;
  public static final int INDEX_CIRCLE_RADIUS = 0;
  public static final int INDEX_CIRCLE_CENTER_X = 1;
  public static final int INDEX_CIRCLE_CENTER_Y = 2;

  // Ellipse
  public static final int PARAMS_LEN_ELLIPSE = 4;
  public static final int INDEX_ELLIPSE_RADIUS_X = 0;
  public static final int INDEX_ELLIPSE_RADIUS_Y = 1;
  public static final int INDEX_ELLIPSE_CENTER_X = 2;
  public static final int INDEX_ELLIPSE_CENTER_Y = 3;

  // Super Ellipse
  public static final int PARAMS_LEN_SUPER_ELLIPSE = 4;
  public static final int INDEX_SUPER_ELLIPSE_RADIUS_X = 0;
  public static final int INDEX_SUPER_ELLIPSE_RADIUS_Y = 1;
  public static final int INDEX_SUPER_ELLIPSE_CENTER_X = 2;
  public static final int INDEX_SUPER_ELLIPSE_CENTER_Y = 3;
  public static final int PARAMS_LEN_SUPER_ELLIPSE_EXPONENTS = 2;
  public static final int INDEX_SUPER_ELLIPSE_EXPONENT_X = 0;
  public static final int INDEX_SUPER_ELLIPSE_EXPONENT_Y = 1;

  private static final int INDEX_BASIC_SHAPE_TYPE = 0;
  private static final int RAW_PARAMS_LEN_INSET_RECT = 9;
  private static final int RAW_PARAMS_LEN_INSET_ROUND = 25;
  private static final int RAW_PARAMS_LEN_INSET_SUPER_ELLIPSE = 27;
  private static final int PARAMS_LEN_INSET = 4;

  private static Matrix sDensityScale = null;
  private Path mPath = null;
  public BasicShape(String data, float scaledDensity) {
    mType = StyleConstants.BASIC_SHAPE_TYPE_PATH;
    try {
      mPath = PathParser.createPathFromPathData(data);
    } catch (RuntimeException e) {
      LLog.e(
          BASIC_SHAPE_TAG, "Create path from data string failed. Check the path string. \n" + data);
    }
    if (mPath == null) {
      LLog.e(BASIC_SHAPE_TAG, "Invalid path data string: " + data);
      return;
    }
    // Unit should always be user units, scale according current viewport's density.
    if (sDensityScale == null) {
      sDensityScale = new Matrix();
    }
    sDensityScale.setScale(scaledDensity, scaledDensity);
    mPath.transform(sDensityScale);
  }
  private final static double sqrt_2 = Math.sqrt(2);
  private int mType;
  // Size for current path.
  private int mWidth;
  private int mHeight;

  public Length[] params = null;
  public double[] exponents = null;
  private BorderRadius mCornerRadius = null;

  private int mCornerType = 0;
  private static final int CORNER_RECT = 1;
  private static final int CORNER_ROUNDED = 2;
  private static final int CORNER_SUPER_ELLIPTICAL = 3;

  public BasicShape(int type) {
    mType = type;
  }

  public static class Length {
    public double mVal;
    public int mUnit;
    public Length(double val, int unit) {
      mVal = val;
      mUnit = unit;
    }
  }

  public Path getPath(int width, int height) {
    if (mType == StyleConstants.BASIC_SHAPE_TYPE_PATH) {
      return mPath;
    }
    if (mType == StyleConstants.BASIC_SHAPE_TYPE_UNKNOWN) {
      return null;
    }
    if (width == mWidth && height == mHeight && mPath != null) {
      return mPath;
    }
    mWidth = width;
    mHeight = height;
    doDrawPath(width, height);
    return mPath;
  }

  /**
   * Implementation of the path generation. Implement it in subclass.
   * @param width viewport's width
   * @param height viewport's height
   */
  protected void doDrawPath(int width, int height) {
    if (mPath == null) {
      mPath = new Path();
    } else {
      mPath.reset();
    }
    switch (mType) {
      case StyleConstants.BASIC_SHAPE_TYPE_UNKNOWN:
        break;
      case StyleConstants.BASIC_SHAPE_TYPE_CIRCLE: {
        if (params == null || params.length != PARAMS_LEN_CIRCLE) {
          break;
        }
        // percentage r = (sqrt(w^2 + h^2) / sqrt_2) * <percentage>
        float r = (float) lengthToDouble(
            params[INDEX_CIRCLE_RADIUS], Math.sqrt(width * width + height * height) / sqrt_2);
        float x = (float) lengthToDouble(params[INDEX_CIRCLE_CENTER_X], width);
        float y = (float) lengthToDouble(params[INDEX_CIRCLE_CENTER_Y], height);
        mPath.addCircle(x, y, r, Path.Direction.CW);
        break;
      }
      case StyleConstants.BASIC_SHAPE_TYPE_ELLIPSE: {
        if (params == null || params.length != PARAMS_LEN_ELLIPSE) {
          break;
        }
        // percentage r = (sqrt(w^2 + h^2) / sqrt_2) * <percentage>
        float rx = (float) lengthToDouble(params[INDEX_ELLIPSE_RADIUS_X], width);
        float ry = (float) lengthToDouble(params[INDEX_ELLIPSE_RADIUS_Y], width);
        float x = (float) lengthToDouble(params[INDEX_ELLIPSE_CENTER_X], width);
        float y = (float) lengthToDouble(params[INDEX_ELLIPSE_CENTER_Y], height);
        if (rx == 0 && ry == 0) {
          // Do nothing
          break;
        }
        // Add ellipse to the target rect at

        // _______________________
        // |         |         | |
        // |______(cx,cy)______| 2 * ry
        // |         |         | |
        // |_________|_________|_|
        // |        x * rx       |
        mPath.addOval(x - rx, y - ry, x + rx, y + ry, Path.Direction.CW);
        break;
      }
      case StyleConstants.BASIC_SHAPE_TYPE_SUPER_ELLIPSE: {
        if (params == null || params.length != PARAMS_LEN_SUPER_ELLIPSE || exponents == null
            || exponents.length != PARAMS_LEN_SUPER_ELLIPSE_EXPONENTS) {
          break;
        }
        float rx = (float) lengthToDouble(params[INDEX_SUPER_ELLIPSE_RADIUS_X], width);
        float ry = (float) lengthToDouble(params[INDEX_SUPER_ELLIPSE_RADIUS_Y], width);
        float cx = (float) lengthToDouble(params[INDEX_SUPER_ELLIPSE_CENTER_X], width);
        float cy = (float) lengthToDouble(params[INDEX_SUPER_ELLIPSE_CENTER_Y], height);
        float ex = (float) exponents[INDEX_SUPER_ELLIPSE_EXPONENT_X];
        float ey = (float) exponents[INDEX_SUPER_ELLIPSE_EXPONENT_Y];
        if (rx == 0 && ry == 0) {
          // Nothing to do, keep the path empty.
          break;
        }
        // Add super-ellipse to the target position cx, cy
        for (int i = 1; i <= 4; i++) {
          addLameCurveToPath(mPath, rx, ry, cx, cy, ex, ey, i);
        }
        mPath.close();
        break;
      }
      case StyleConstants.BASIC_SHAPE_TYPE_INSET: {
        doDrawBasicShapeInset(width, height);
        break;
      }
    }
  }

  // generate path according to shape attributes.
  private void doDrawBasicShapeInset(int width, int height) {
    // Need at least four sides inset values.
    if (params == null || params.length != PARAMS_LEN_INSET) {
      return;
    }
    double top = lengthToDouble(params[0], height);
    double right = lengthToDouble(params[1], width);
    double bottom = lengthToDouble(params[2], height);
    double left = lengthToDouble(params[3], width);

    // Adjust inset value, values are proportionally reduce the inset effect to 100% if the pair of
    // insets in either dimension add up to more than the side length.
    double vInset = top + bottom;
    double hInset = left + right;
    if (vInset != 0 && (vInset > height)) {
      double s = height / vInset;
      top *= s;
      bottom *= s;
    }
    if (hInset != 0 && (hInset > width)) {
      double s = width / hInset;
      left *= s;
      right *= s;
    }

    RectF insetRect =
        new RectF((float) left, (float) top, (float) (width - right), (float) (height - bottom));
    switch (mCornerType) {
      case CORNER_RECT:
        // Add the rect to path directly, no rounded corner.
        mPath.addRect(insetRect, Path.Direction.CW);
        break;
      case CORNER_ROUNDED:
        // Add rounded corner.
        mCornerRadius.updateSize(insetRect.width(), insetRect.height());
        mPath.addRoundRect(insetRect, mCornerRadius.getArray(), Path.Direction.CW);
        break;
      case CORNER_SUPER_ELLIPTICAL:
        mCornerRadius.updateSize(insetRect.width(), insetRect.height());
        float[] radius = mCornerRadius.getArray();
        if (radius.length < 8)
          return;
        // Add ellipse to the target rect at cx cy
        // bottom-right corner
        float rx = radius[4];
        float ry = radius[5];
        float cx = insetRect.right - rx;
        float cy = insetRect.bottom - ry;
        float ex = (float) exponents[0];
        float ey = (float) exponents[1];
        addLameCurveToPath(mPath, rx, ry, cx, cy, ex, ey, 1);

        // bottom-left corner
        rx = radius[6];
        rx = radius[7];
        cx = insetRect.left + rx;
        cy = insetRect.bottom - ry;
        addLameCurveToPath(mPath, rx, ry, cx, cy, ex, ey, 2);

        // top-left corner
        rx = radius[0];
        ry = radius[1];
        cx = insetRect.left + rx;
        cy = insetRect.top + ry;
        addLameCurveToPath(mPath, rx, ry, cx, cy, ex, ey, 3);

        // top-right corner
        rx = radius[2];
        ry = radius[3];
        cx = insetRect.right - rx;
        cy = insetRect.top + ry;
        addLameCurveToPath(mPath, rx, ry, cx, cy, ex, ey, 4);
        mPath.close();
        break;
    }
  }

  private double lengthToDouble(Length l, double context) {
    if (l == null) {
      return 0;
    }
    return l.mUnit == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT ? l.mVal * context : l.mVal;
  }

  // Add Lamé curve in the target quadrant to the target path.
  private static void addLameCurveToPath(
      Path path, float rx, float ry, float cx, float cy, float ex, float ey, int quadrant) {
    double cosI, sinI, x, y;
    float fx = (quadrant == 1 || quadrant == 4) ? 1 : -1;
    float fy = (quadrant == 1 || quadrant == 2) ? 1 : -1;
    for (float i = (float) (Math.PI / 2 * (quadrant - 1)); i < Math.PI / 2 * quadrant; i += 0.01) {
      // abs for cos and sin
      cosI = fx * Math.cos(i);
      sinI = fy * Math.sin(i);
      x = fx * rx * Math.pow(cosI, 2 / ex) + cx;
      y = fy * ry * Math.pow(sinI, 2 / ey) + cy;
      if (i == 0) {
        path.moveTo((float) x, (float) y);
      } else {
        path.lineTo((float) x, (float) y);
      }
    }
  }
}
