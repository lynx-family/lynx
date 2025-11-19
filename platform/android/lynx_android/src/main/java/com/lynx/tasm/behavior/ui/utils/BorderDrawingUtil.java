/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * <p>
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
// Changes made by Lynx:
// 1. Extracted border drawing logic from BackgroundDrawable into a standalone utility class
// 2. Simplified API to accept pre-processed border data arrays instead of functional interfaces
// 3. Added fast path optimization for uniform borders using single stroke operation

package com.lynx.tasm.behavior.ui.utils;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import androidx.annotation.NonNull;

/**
 * Utility class for drawing rectangular borders.
 * Provides general functions for stroking rectangle borders according to bounds, width array, color
 * array and style array.
 */
public class BorderDrawingUtil {
  /**
   * Draws rectangular borders on a canvas with the specified parameters.
   * This is a general utility function that can be used by any drawable that needs to draw
   * rectangular borders.
   *
   * @param canvas       The canvas to draw on
   * @param paint        The paint object to use for drawing (will be modified during drawing)
   * @param bounds       The bounds of the rectangle to draw borders around
   * @param borderWidths Array of calculated border widths for [left, top, right, bottom] in pixels
   * @param borderColors Array of border colors for [left, top, right, bottom]
   * @param borderStyles Array of border styles for [left, top, right, bottom], cannot be null
   */
  public static void drawRectangularBorders(Canvas canvas, Paint paint, Rect bounds,
      int[] borderWidths, int[] borderColors, @NonNull BorderStyle[] borderStyles) {
    if (borderWidths == null || borderWidths.length != 4 || borderColors == null
        || borderColors.length != 4 || borderStyles.length != 4) {
      return;
    }

    final int borderLeftWidth = borderWidths[Spacing.LEFT];
    final int borderTopWidth = borderWidths[Spacing.TOP];
    final int borderRightWidth = borderWidths[Spacing.RIGHT];
    final int borderBottomWidth = borderWidths[Spacing.BOTTOM];

    final boolean borderWidthToDraw = (borderLeftWidth > 0 || borderRightWidth > 0
        || borderTopWidth > 0 || borderBottomWidth > 0);

    // maybe draw borders?
    if (borderWidthToDraw) {
      int colorLeft = borderColors[Spacing.LEFT];
      int colorTop = borderColors[Spacing.TOP];
      int colorRight = borderColors[Spacing.RIGHT];
      int colorBottom = borderColors[Spacing.BOTTOM];

      int left = bounds.left;
      int top = bounds.top;

      paint.setAntiAlias(false);
      paint.setStyle(Paint.Style.STROKE);

      // Check for fast path to border drawing.
      int fastBorderColor = fastBorderCompatibleColorOrZero(borderLeftWidth, borderTopWidth,
          borderRightWidth, borderBottomWidth, colorLeft, colorTop, colorRight, colorBottom);

      if (fastBorderColor != 0 && toDrawBorderUseSameStyle(borderStyles)) {
        if (Color.alpha(fastBorderColor) != 0) {
          final int right = bounds.right, bottom = bounds.bottom;
          final BorderStyle borderStyle = borderStyles[Spacing.TOP];

          // Check if all border widths are the same for true fast path
          if (borderLeftWidth == borderTopWidth && borderTopWidth == borderRightWidth
              && borderRightWidth == borderBottomWidth && borderStyle == BorderStyle.SOLID) {
            // True fast path: single stroke for uniform border
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(borderLeftWidth);
            paint.setColor(fastBorderColor);

            // Draw the border as a single stroke around the bounds
            android.graphics.RectF strokeRect = new android.graphics.RectF(
                left + borderLeftWidth * 0.5f, top + borderLeftWidth * 0.5f,
                right - borderLeftWidth * 0.5f, bottom - borderLeftWidth * 0.5f);

            canvas.drawRect(strokeRect, paint);

          } else {
            // Fast path with different widths: draw each side separately but with same style/color
            if (borderTopWidth > 0) {
              final float topCenter = top + borderTopWidth * 0.5f;
              // should not cover start of the next section, or if alpha is not 255, the color
              // will be darken
              final float endTop = right - (Math.max(borderRightWidth, 0));
              borderStyle.strokeBorderLine(canvas, paint, Spacing.TOP, borderTopWidth,
                  fastBorderColor, left, topCenter, endTop, topCenter, right - left,
                  borderTopWidth);
            }
            if (borderRightWidth > 0) {
              final float rightCenter = right - borderRightWidth * 0.5f;
              final float endRight = bottom - (Math.max(borderBottomWidth, 0));
              borderStyle.strokeBorderLine(canvas, paint, Spacing.RIGHT, borderRightWidth,
                  fastBorderColor, rightCenter, top, rightCenter, endRight, bottom - top,
                  borderRightWidth);
            }
            if (borderBottomWidth > 0) {
              final float bottomCenter = bottom - borderBottomWidth * 0.5f;
              final float endBottom = left + (Math.max(borderLeftWidth, 0));
              borderStyle.strokeBorderLine(canvas, paint, Spacing.BOTTOM, borderBottomWidth,
                  fastBorderColor, right, bottomCenter, endBottom, bottomCenter, right - left,
                  borderBottomWidth);
            }
            if (borderLeftWidth > 0) {
              final float leftCenter = left + borderLeftWidth * 0.5f;
              final float endLeft = top + (Math.max(borderTopWidth, 0));
              borderStyle.strokeBorderLine(canvas, paint, Spacing.LEFT, borderLeftWidth,
                  fastBorderColor, leftCenter, bottom, leftCenter, endLeft, bottom - top,
                  borderLeftWidth);
            }
          }
        }
      } else {
        // If the path drawn previously is of the same color,
        // there would be a slight white space between borders
        // with anti-alias set to true.
        // Therefore we need to disable anti-alias, and
        // after drawing is done, we will re-enable it.
        int width = bounds.width();
        int height = bounds.height();

        if (borderTopWidth > 0 && Color.alpha(colorTop) != 0) {
          final float x2 = left + borderLeftWidth;
          final float y2 = top + borderTopWidth;
          final float x3 = left + width - borderRightWidth;
          final float y3 = top + borderTopWidth;
          final float x4 = left + width;
          final float y5 = top + borderTopWidth * 0.5f;
          canvas.save();
          clipQuadrilateral(canvas, (float) left, (float) top, x2, y2, x3, y3, x4, (float) top);
          borderStyles[Spacing.TOP].strokeBorderLine(canvas, paint, Spacing.TOP, borderTopWidth,
              colorTop, (float) left, y5, x4, y5, width, borderTopWidth);
          canvas.restore();
        }

        if (borderRightWidth > 0 && Color.alpha(colorRight) != 0) {
          final float x1 = left + width;
          final float x2 = left + width;
          final float y2 = top + height;
          final float x3 = left + width - borderRightWidth;
          final float y3 = top + height - borderBottomWidth;
          final float x4 = left + width - borderRightWidth;
          final float y4 = top + borderTopWidth;
          final float x5 = left + width - borderRightWidth * 0.5f;
          canvas.save();
          clipQuadrilateral(canvas, x1, (float) top, x2, y2, x3, y3, x4, y4);
          borderStyles[Spacing.RIGHT].strokeBorderLine(canvas, paint, Spacing.RIGHT,
              borderRightWidth, colorRight, x5, (float) top, x5, y2, height, borderRightWidth);
          canvas.restore();
        }

        if (borderBottomWidth > 0 && Color.alpha(colorBottom) != 0) {
          final float y1 = top + height;
          final float x2 = left + width;
          final float y2 = top + height;
          final float x3 = left + width - borderRightWidth;
          final float y3 = top + height - borderBottomWidth;
          final float x4 = left + borderLeftWidth;
          final float y4 = top + height - borderBottomWidth;
          final float y5 = top + height - borderBottomWidth * 0.5f;
          canvas.save();
          clipQuadrilateral(canvas, (float) left, y1, x2, y2, x3, y3, x4, y4);
          borderStyles[Spacing.BOTTOM].strokeBorderLine(canvas, paint, Spacing.BOTTOM,
              borderBottomWidth, colorBottom, x2, y5, (float) left, y5, width, borderBottomWidth);
          canvas.restore();
        }

        if (borderLeftWidth > 0 && Color.alpha(colorLeft) != 0) {
          final float x2 = left + borderLeftWidth;
          final float y2 = top + borderTopWidth;
          final float x3 = left + borderLeftWidth;
          final float y3 = top + height - borderBottomWidth;
          final float y4 = top + height;
          final float x5 = left + borderLeftWidth * 0.5f;
          canvas.save();
          clipQuadrilateral(canvas, (float) left, (float) top, x2, y2, x3, y3, (float) left, y4);
          borderStyles[Spacing.LEFT].strokeBorderLine(canvas, paint, Spacing.LEFT, borderLeftWidth,
              colorLeft, x5, y4, x5, (float) top, height, borderLeftWidth);
          canvas.restore();
        }
      }
    }
    paint.setAntiAlias(true);
  }

  /**
   * Quickly determine if all the set border colors are equal. Bitwise AND all the
   * set colors together, then OR them all together. If the AND and the OR are the
   * same, then the colors are compatible, so return this color.
   * <p>
   * Used to avoid expensive path creation and expensive calls to canvas.drawPath
   *
   * @return A compatible border color, or zero if the border colors are not
   * compatible.
   */
  private static int fastBorderCompatibleColorOrZero(int borderLeft, int borderTop, int borderRight,
      int borderBottom, int colorLeft, int colorTop, int colorRight, int colorBottom) {
    int andSmear = (borderLeft > 0 ? colorLeft : ALL_BITS_SET)
        & (borderTop > 0 ? colorTop : ALL_BITS_SET) & (borderRight > 0 ? colorRight : ALL_BITS_SET)
        & (borderBottom > 0 ? colorBottom : ALL_BITS_SET);
    int orSmear = (borderLeft > 0 ? colorLeft : ALL_BITS_UNSET)
        | (borderTop > 0 ? colorTop : ALL_BITS_UNSET)
        | (borderRight > 0 ? colorRight : ALL_BITS_UNSET)
        | (borderBottom > 0 ? colorBottom : ALL_BITS_UNSET);
    return andSmear == orSmear ? andSmear : 0;
  }

  private static boolean toDrawBorderUseSameStyle(@NonNull BorderStyle[] borderStyles) {
    if (borderStyles[0] != borderStyles[1] || borderStyles[1] != borderStyles[2]
        || borderStyles[2] != borderStyles[3]) {
      return false;
    }
    return borderStyles[0].isSolidDashedOrDotted();
  }

  private static void clipQuadrilateral(Canvas canvas, float x1, float y1, float x2, float y2,
      float x3, float y3, float x4, float y4) {
    // Create a path for the quadrilateral and clip the canvas
    Path path = new Path();
    path.reset();
    path.moveTo(x1, y1);
    path.lineTo(x2, y2);
    path.lineTo(x3, y3);
    path.lineTo(x4, y4);
    path.lineTo(x1, y1);
    canvas.clipPath(path);
  }

  // ~0 == 0xFFFFFFFF, all bits set to 1.
  private static final int ALL_BITS_SET = ~0;
  // 0 == 0x00000000, all bits set to 0.
  private static final int ALL_BITS_UNSET = 0;
}
