// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.utils;

import android.graphics.Canvas;
import android.graphics.DashPathEffect;
import android.graphics.Paint;
import android.graphics.PathEffect;
import androidx.annotation.Nullable;

public enum BorderStyle {
  SOLID,
  DASHED,
  DOTTED,
  DOUBLE,
  GROOVE,
  RIDGE,
  INSET,
  OUTSET,
  HIDDEN,
  NONE;

  public @Nullable PathEffect getPathEffect(float borderWidth) {
    switch (this) {
      case DASHED:
        return new DashPathEffect(
            new float[] {borderWidth * 3, borderWidth * 3, borderWidth * 3, borderWidth * 3}, 0);

      case DOTTED:
        return new DashPathEffect(
            new float[] {borderWidth, borderWidth, borderWidth, borderWidth}, 0);

      default:
        return null;
    }
  }

  public @Nullable PathEffect getPathEffectAutoAdjust(float borderWidth, float borderLength) {
    if (this != DASHED && this != DOTTED) {
      return null;
    }

    float sectionLen = (borderWidth >= 1 ? borderWidth : 1) * (this == DOTTED ? 2 : 6) * 0.5f;
    int newSectionCount = ((int) ((borderLength / sectionLen - 0.5f) * 0.5f)) * 2 + 1;
    if (newSectionCount <= 1) {
      return null;
    }

    return new DashPathEffect(
        new float[] {borderLength / newSectionCount, borderLength / newSectionCount}, 0);
  }

  public boolean isSolidDashedOrDotted() {
    switch (this) {
      case SOLID:
      case DASHED:
      case DOTTED:
        return true;
      default:
        break;
    }
    return false;
  }

  public static BorderStyle parse(int style) {
    if (style < 0 || style > BorderStyle.NONE.ordinal()) {
      return null;
    }
    return BorderStyle.values()[style];
  }

  private static int darkenColor(int color) {
    return ((color & 0x00FEFEFE) >> 1) | (color & 0xFF000000);
  }

  private static int brightColor(int color) {
    return (color | 0x00808080);
  }

  private static boolean isDarkColor(int color) {
    return (color & 0x00F0F0F0) == 0;
  }

  private void strokeBorderMoreLines(Canvas canvas, Paint paint, int borderPosition,
      float borderWidth, float borderOffset, int color0, int color1, float startX, float startY,
      float stopX, float stopY) {
    paint.setPathEffect(null);
    paint.setStrokeWidth(borderWidth);

    for (int i = -1; i <= 1; i += 2) {
      float xOffset = 0, yOffset = 0;
      int color = 0;
      switch (borderPosition) {
        case Spacing.TOP:
          yOffset = borderOffset * i;
          color = (i == 1 ? color0 : color1);
          break;
        case Spacing.RIGHT:
          xOffset = -borderOffset * i;
          color = (i == 1 ? color1 : color0);
          break;
        case Spacing.BOTTOM:
          yOffset = -borderOffset * i;
          color = (i == 1 ? color1 : color0);
          break;
        case Spacing.LEFT:
          xOffset = borderOffset * i;
          color = (i == 1 ? color0 : color1);
          break;
      }

      paint.setColor(color);
      canvas.drawLine(startX + xOffset, startY + yOffset, stopX + xOffset, stopY + yOffset, paint);
    }
  }

  public void strokeBorderLine(Canvas canvas, Paint paint, int borderPosition, float borderWidth,
      int color, float startX, float startY, float stopX, float stopY, float borderLength,
      float borderMeasureWidth) {
    PathEffect pathEffect = null;
    switch (this) {
      case NONE:
      case HIDDEN:
        return;

      case DASHED:
      case DOTTED:
        pathEffect = getPathEffectAutoAdjust(borderMeasureWidth, borderLength);
        break;

      case SOLID:
        break;
      case INSET: {
        if (isDarkColor(color)) {
          if (borderPosition == Spacing.BOTTOM || borderPosition == Spacing.RIGHT) {
            color = brightColor(color);
          }
        } else {
          if (borderPosition == Spacing.TOP || borderPosition == Spacing.LEFT) {
            color = darkenColor(color);
          }
        }
      } break;
      case OUTSET:
        if (isDarkColor(color)) {
          if (borderPosition == Spacing.TOP || borderPosition == Spacing.LEFT) {
            color = brightColor(color);
          }
        } else {
          if (borderPosition == Spacing.BOTTOM || borderPosition == Spacing.RIGHT) {
            color = darkenColor(color);
          }
        }
        break;

      case DOUBLE:
        strokeBorderMoreLines(canvas, paint, borderPosition, borderWidth / 3,
            borderMeasureWidth / 3, color, color, startX, startY, stopX, stopY);
        return;
      case GROOVE:
        if (isDarkColor(color)) {
          strokeBorderMoreLines(canvas, paint, borderPosition, borderWidth / 2,
              borderMeasureWidth / 4, brightColor(color), color, startX, startY, stopX, stopY);
        } else {
          strokeBorderMoreLines(canvas, paint, borderPosition, borderWidth / 2,
              borderMeasureWidth / 4, color, darkenColor(color), startX, startY, stopX, stopY);
        }
        return;
      case RIDGE:
        if (isDarkColor(color)) {
          strokeBorderMoreLines(canvas, paint, borderPosition, borderWidth / 2,
              borderMeasureWidth / 4, color, brightColor(color), startX, startY, stopX, stopY);
        } else {
          strokeBorderMoreLines(canvas, paint, borderPosition, borderWidth / 2,
              borderMeasureWidth / 4, darkenColor(color), color, startX, startY, stopX, stopY);
        }
        return;
    }

    paint.setStyle(Paint.Style.STROKE);
    paint.setColor(color);
    paint.setPathEffect(pathEffect);
    paint.setStrokeWidth(borderWidth);
    canvas.drawLine(startX, startY, stopX, stopY, paint);
    paint.setPathEffect(null);
  }
}
