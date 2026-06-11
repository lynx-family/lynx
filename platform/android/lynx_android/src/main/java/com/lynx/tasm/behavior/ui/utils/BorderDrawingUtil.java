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
import android.graphics.PathEffect;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import androidx.annotation.NonNull;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.render.RoundedRectangle;

/**
 * Utility class for drawing rectangular borders.
 * Provides general functions for stroking rectangle borders according to bounds, width array, color
 * array and style array.
 */
public class BorderDrawingUtil {
  private static String TAG = "BorderDrawingUtil";

  // TODO(songshourui.null): Let BackgroundDrawable use this method later.
  /**
   * Draws rounded rectangle borders on a canvas with the specified parameters.
   * This is a general utility function that can be used by any drawable that needs to draw
   * rounded rectangle borders.
   *
   * @param canvas       The canvas to draw on
   * @param paint        The paint object to use for drawing (will be modified during drawing)
   * @param outBox       The rounded rectangle to draw outer border around
   * @param innerBox     The rounded rectangle to draw inner border around
   * @param borderColors Array of border colors for [left, top, right, bottom]
   * @param borderStyles Array of border styles for [left, top, right, bottom], cannot be null
   */
  public static void drawBorders(Canvas canvas, Paint paint, RoundedRectangle outBox,
      RoundedRectangle innerBox, int[] borderColors, @NonNull BorderStyle[] borderStyles) {
    int[] borderWidths = new int[] {(int) (innerBox.getRectF().left - outBox.getRectF().left),
        (int) (innerBox.getRectF().top - outBox.getRectF().top),
        (int) (outBox.getRectF().right - innerBox.getRectF().right),
        (int) (outBox.getRectF().bottom - innerBox.getRectF().bottom)};

    if (outBox.hasBorderRadius() || innerBox.hasBorderRadius()) {
      drawRoundedBorders(canvas, paint, outBox, innerBox, borderWidths, borderColors, borderStyles);
    } else {
      drawRectangularBorders(
          canvas, paint, outBox.getRect(), borderWidths, borderColors, borderStyles);
    }
  }

  // TODO(songshourui.null): Let BackgroundDrawable use drawBorders method later, and make this
  // method private.
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
   * Draws rounded rectangle borders on a canvas with the specified parameters.
   * This is a general utility function that can be used by any drawable that needs to draw
   * rounded rectangle borders.
   *
   * @param canvas       The canvas to draw on
   * @param paint        The paint object to use for drawing (will be modified during drawing)
   * @param outBounds    The rounded rectangle to draw outer border around
   * @param innerBounds  The rounded rectangle to draw inner border around
   * @param borderWidths Array of calculated border widths for [left, top, right, bottom] in pixels
   * @param borderColors Array of border colors for [left, top, right, bottom]
   * @param borderStyles Array of border styles for [left, top, right, bottom], cannot be null
   */
  private static void drawRoundedBorders(Canvas canvas, Paint paint, RoundedRectangle outBounds,
      RoundedRectangle innerBounds, int[] borderWidths, int[] borderColors,
      @NonNull BorderStyle[] borderStyles) {
    canvas.save();

    final int borderLeftWidth = borderWidths[Spacing.LEFT];
    final int borderTopWidth = borderWidths[Spacing.TOP];
    final int borderRightWidth = borderWidths[Spacing.RIGHT];
    final int borderBottomWidth = borderWidths[Spacing.BOTTOM];

    final int borderColorLeft = borderColors[Spacing.LEFT];
    final int borderColorTop = borderColors[Spacing.TOP];
    final int borderColorRight = borderColors[Spacing.RIGHT];
    final int borderColorBottom = borderColors[Spacing.BOTTOM];

    if (borderLeftWidth > 0 || borderTopWidth > 0 || borderRightWidth > 0
        || borderBottomWidth > 0) {
      // If it's a full and even border draw inner rect path with stroke

      final boolean isBorderColorSame = (borderColorLeft == borderColorRight
          && borderColorLeft == borderColorTop && borderColorLeft == borderColorBottom);

      final boolean isBorderWidthSame = (borderTopWidth == borderLeftWidth
          && borderBottomWidth == borderLeftWidth && borderRightWidth == borderLeftWidth);

      if (isBorderWidthSame && isBorderColorSame && toDrawBorderUseSameStyle(borderStyles)) {
        BorderStyle borderStyle = borderStyles[Spacing.LEFT];
        // When the outer radius is smaller than half the border width, stroking a
        // center rounded rect clamps the center radius to 0 and draws square outer
        // corners, losing the intended outer border radius. Use a fill path that
        // subtracts the inner box from the outer box instead.
        if (borderStyle == BorderStyle.SOLID && shouldUseFillBorderPath(outBounds, borderWidths)) {
          drawRoundedBorderArea(canvas, paint, outBounds, innerBounds, borderColorLeft);
        } else {
          strokeCenterDrawPath(canvas, paint, borderStyle, Spacing.LEFT, borderColorLeft,
              borderLeftWidth, borderLeftWidth, outBounds, innerBounds);
        }
      }
      // In the case of uneven border widths/colors draw quadrilateral in each
      // direction
      else {
        final RectF outerClipRect = outBounds.getRectF();
        final float left = outerClipRect.left;
        final float right = outerClipRect.right;
        final float top = outerClipRect.top;
        final float bottom = outerClipRect.bottom;

        final RectF innerClipRect = innerBounds.getRectF();
        final PointF innerTopLeft = new PointF(innerClipRect.left, innerClipRect.top);
        BackgroundDrawable.getEllipseIntersectionWithLine(innerClipRect.left, innerClipRect.top,
            innerClipRect.left + 2 * innerBounds.getTopLeftRadiusX(),
            innerClipRect.top + 2 * innerBounds.getTopLeftRadiusY(), outerClipRect.left,
            outerClipRect.top, innerClipRect.left, innerClipRect.top, innerTopLeft);

        final PointF innerTopRight = new PointF(innerClipRect.right, innerClipRect.top);
        BackgroundDrawable.getEllipseIntersectionWithLine(
            innerClipRect.right - 2 * innerBounds.getTopRightRadiusX(), innerClipRect.top,
            innerClipRect.right, innerClipRect.top + 2 * innerBounds.getTopRightRadiusY(),
            outerClipRect.right, outerClipRect.top, innerClipRect.right, innerClipRect.top,
            innerTopRight);

        final PointF innerBottomRight = new PointF(innerClipRect.right, innerClipRect.bottom);
        BackgroundDrawable.getEllipseIntersectionWithLine(
            innerClipRect.right - 2 * innerBounds.getBottomRightRadiusX(),
            innerClipRect.bottom - 2 * innerBounds.getBottomRightRadiusY(), innerClipRect.right,
            innerClipRect.bottom, outerClipRect.right, outerClipRect.bottom, innerClipRect.right,
            innerClipRect.bottom, innerBottomRight);

        final PointF innerBottomLeft = new PointF(innerClipRect.left, innerClipRect.bottom);
        BackgroundDrawable.getEllipseIntersectionWithLine(innerClipRect.left,
            innerClipRect.bottom - 2 * innerBounds.getBottomLeftRadiusY(),
            innerClipRect.left + 2 * innerBounds.getBottomLeftRadiusX(), innerClipRect.bottom,
            outerClipRect.left, outerClipRect.bottom, innerClipRect.left, innerClipRect.bottom,
            innerBottomLeft);

        if (borderTopWidth > 0 && Color.alpha(borderColorTop) != 0) {
          final float x1 = left;
          final float y1 = top;
          final float x2 = innerTopLeft.x;
          final float y2 = innerTopLeft.y;
          final float x3 = innerTopRight.x;
          final float y3 = innerTopRight.y;
          final float x4 = right;
          final float y4 = top;
          boolean toClip = false;
          float w = borderTopWidth;
          if (!isBorderWidthSame) {
            w = Math.max(w, Math.max(borderLeftWidth, borderRightWidth));
            toClip = w - Math.min(borderLeftWidth, borderRightWidth) >= 2;
          }
          canvas.save();
          clipQuadrilateralWithBounds(
              canvas, x1, y1, x2, y2, x3, y3, x4, y4, toClip, outBounds, innerBounds);
          strokeCenterDrawPath(canvas, paint, borderStyles[Spacing.TOP], Spacing.TOP,
              borderColorTop, borderTopWidth, w, outBounds, innerBounds);
          canvas.restore();
        }

        if (borderRightWidth > 0 && borderColorRight != 0) {
          final float x1 = right;
          final float y1 = top;
          final float x2 = innerTopRight.x;
          final float y2 = innerTopRight.y;
          final float x3 = innerBottomRight.x;
          final float y3 = innerBottomRight.y;
          final float x4 = right;
          final float y4 = bottom;
          boolean toClip = false;
          float w = borderRightWidth;
          if (!isBorderWidthSame) {
            w = Math.max(w, Math.max(borderTopWidth, borderBottomWidth));
            toClip = w - Math.min(borderTopWidth, borderBottomWidth) >= 2;
          }
          canvas.save();
          clipQuadrilateralWithBounds(
              canvas, x1, y1, x2, y2, x3, y3, x4, y4, toClip, outBounds, innerBounds);
          strokeCenterDrawPath(canvas, paint, borderStyles[Spacing.RIGHT], Spacing.RIGHT,
              borderColorRight, borderRightWidth, w, outBounds, innerBounds);
          canvas.restore();
        }

        if (borderBottomWidth > 0 && borderColorBottom != 0) {
          final float x1 = left;
          final float y1 = bottom;
          final float x2 = innerBottomLeft.x;
          final float y2 = innerBottomLeft.y;
          final float x3 = innerBottomRight.x;
          final float y3 = innerBottomRight.y;
          final float x4 = right;
          final float y4 = bottom;
          boolean toClip = false;
          float w = borderBottomWidth;
          if (!isBorderWidthSame) {
            w = Math.max(w, Math.max(borderLeftWidth, borderRightWidth));
            toClip = w - Math.min(borderLeftWidth, borderRightWidth) >= 2;
          }
          canvas.save();
          clipQuadrilateralWithBounds(
              canvas, x1, y1, x2, y2, x3, y3, x4, y4, toClip, outBounds, innerBounds);
          strokeCenterDrawPath(canvas, paint, borderStyles[Spacing.BOTTOM], Spacing.BOTTOM,
              borderColorBottom, borderBottomWidth, w, outBounds, innerBounds);
          canvas.restore();
        }

        if (borderLeftWidth > 0 && borderColorLeft != 0) {
          final float x1 = left;
          final float y1 = top;
          final float x2 = innerTopLeft.x;
          final float y2 = innerTopLeft.y;
          final float x3 = innerBottomLeft.x;
          final float y3 = innerBottomLeft.y;
          final float x4 = left;
          final float y4 = bottom;
          boolean toClip = false;
          float w = borderLeftWidth;
          if (!isBorderWidthSame) {
            w = Math.max(w, Math.max(borderTopWidth, borderBottomWidth));
            toClip = w - Math.min(borderTopWidth, borderBottomWidth) >= 2;
          }
          canvas.save();
          clipQuadrilateralWithBounds(
              canvas, x1, y1, x2, y2, x3, y3, x4, y4, toClip, outBounds, innerBounds);
          strokeCenterDrawPath(canvas, paint, borderStyles[Spacing.LEFT], Spacing.LEFT,
              borderColorLeft, borderLeftWidth, w, outBounds, innerBounds);
          canvas.restore();
        }
      }
    }

    canvas.restore();
  }

  /**
   * Returns true when stroking a center rounded rect would clamp the center radius to
   * zero while the outer box still has a non-zero radius. In that case the stroke-only
   * fast path draws square outer corners and loses the intended border radius.
   */
  private static boolean shouldUseFillBorderPath(RoundedRectangle outBounds, int[] borderWidths) {
    if (!outBounds.hasBorderRadius() || borderWidths == null || borderWidths.length != 4) {
      return false;
    }
    float[] radii = outBounds.getBorderRadii();
    if (radii == null || radii.length != 8) {
      return false;
    }
    return (radii[0] > 0 && radii[0] <= borderWidths[Spacing.LEFT] * 0.5f)
        || (radii[1] > 0 && radii[1] <= borderWidths[Spacing.TOP] * 0.5f)
        || (radii[2] > 0 && radii[2] <= borderWidths[Spacing.RIGHT] * 0.5f)
        || (radii[3] > 0 && radii[3] <= borderWidths[Spacing.TOP] * 0.5f)
        || (radii[4] > 0 && radii[4] <= borderWidths[Spacing.RIGHT] * 0.5f)
        || (radii[5] > 0 && radii[5] <= borderWidths[Spacing.BOTTOM] * 0.5f)
        || (radii[6] > 0 && radii[6] <= borderWidths[Spacing.LEFT] * 0.5f)
        || (radii[7] > 0 && radii[7] <= borderWidths[Spacing.BOTTOM] * 0.5f);
  }

  /**
   * Draws the border area as the region between the outer rounded rectangle and the
   * inner rectangle using an even-odd fill path. This preserves the outer radius even
   * when the inner radius has been clamped to zero by a thick border.
   */
  private static void drawRoundedBorderArea(Canvas canvas, Paint paint, RoundedRectangle outBounds,
      RoundedRectangle innerBounds, int borderColor) {
    Path path = new Path();
    path.setFillType(Path.FillType.EVEN_ODD);
    path.addRoundRect(outBounds.getRectF(), outBounds.getBorderRadii(), Path.Direction.CW);

    if (innerBounds.hasBorderRadius()) {
      path.addRoundRect(innerBounds.getRectF(), innerBounds.getBorderRadii(), Path.Direction.CCW);
    } else {
      path.addRect(innerBounds.getRectF(), Path.Direction.CCW);
    }

    paint.setStyle(Paint.Style.FILL);
    paint.setColor(borderColor);
    paint.setPathEffect(null);
    paint.setAntiAlias(true);
    canvas.drawPath(path, paint);
  }

  private static void strokeCenterDrawPathMoreLines(Canvas canvas, Paint paint, int borderPosition,
      float borderWidth, int color0, int color1, boolean isDoubleStyle, RoundedRectangle outBounds,
      RoundedRectangle innerBounds) {
    paint.setPathEffect(null);
    paint.setStyle(Paint.Style.STROKE);
    paint.setStrokeWidth(borderWidth);

    final boolean isTopLeft = (borderPosition == Spacing.TOP || borderPosition == Spacing.LEFT);

    paint.setColor(ColorUtil.multiplyColorAlpha(isTopLeft ? color1 : color0, 255));
    BackgroundDrawable.RoundRectPath centerPathOuter =
        new BackgroundDrawable.RoundRectPath(outBounds, innerBounds,
            isDoubleStyle ? BackgroundDrawable.RoundRectPath.Pos.OUTER3
                          : BackgroundDrawable.RoundRectPath.Pos.OUTER2);
    centerPathOuter.drawToCanvas(canvas, paint);

    paint.setColor(ColorUtil.multiplyColorAlpha(isTopLeft ? color0 : color1, 255));
    BackgroundDrawable.RoundRectPath centerPathInner =
        new BackgroundDrawable.RoundRectPath(outBounds, innerBounds,
            isDoubleStyle ? BackgroundDrawable.RoundRectPath.Pos.INNER3
                          : BackgroundDrawable.RoundRectPath.Pos.INNER2);
    centerPathInner.drawToCanvas(canvas, paint);
  }

  private static void strokeCenterDrawPath(Canvas canvas, Paint paint, BorderStyle borderStyle,
      int borderPosition, int borderColor, float borderWidthForEffect, float borderWidthForStroke,
      RoundedRectangle outBounds, RoundedRectangle innerBounds) {
    PathEffect pathEffectForBorderStyle = null;
    switch (borderStyle) {
      case NONE:
      case HIDDEN:
        return;

      case DASHED:
      case DOTTED:
        pathEffectForBorderStyle = borderStyle.getPathEffect(borderWidthForEffect);
        break;

      case SOLID:
        break;
      case INSET:
        if (borderPosition == Spacing.TOP || borderPosition == Spacing.LEFT) {
          borderColor = BorderStyle.darkenColor(borderColor);
        }
        break;
      case OUTSET:
        if (borderPosition == Spacing.BOTTOM || borderPosition == Spacing.RIGHT) {
          borderColor = BorderStyle.darkenColor(borderColor);
        }
        break;

      case DOUBLE:
        strokeCenterDrawPathMoreLines(canvas, paint, borderPosition, borderWidthForEffect / 3.0f,
            borderColor, borderColor, true, outBounds, innerBounds);
        return;
      case GROOVE:
        strokeCenterDrawPathMoreLines(canvas, paint, borderPosition, borderWidthForEffect / 2.0f,
            borderColor, BorderStyle.darkenColor(borderColor), false, outBounds, innerBounds);
        return;
      case RIDGE:
        strokeCenterDrawPathMoreLines(canvas, paint, borderPosition, borderWidthForEffect / 2.0f,
            BorderStyle.darkenColor(borderColor), borderColor, false, outBounds, innerBounds);
        return;
      default:
        LLog.e(TAG, "Unsupported border style: " + borderStyle);
    }

    paint.setStyle(Paint.Style.STROKE);
    paint.setColor(ColorUtil.multiplyColorAlpha(borderColor, 255));
    paint.setStrokeWidth(borderWidthForStroke);
    paint.setPathEffect(pathEffectForBorderStyle);
    paint.setAntiAlias(true);

    BackgroundDrawable.RoundRectPath centerPath = new BackgroundDrawable.RoundRectPath(
        outBounds, innerBounds, BackgroundDrawable.RoundRectPath.Pos.CENTER);
    centerPath.drawToCanvas(canvas, paint);

    paint.setPathEffect(null);
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

  private static void clipQuadrilateralWithBounds(Canvas canvas, float x1, float y1, float x2,
      float y2, float x3, float y3, float x4, float y4, boolean needClip,
      RoundedRectangle outBounds, RoundedRectangle innerBounds) {
    if (needClip) {
      if (outBounds != null) {
        Path path = new Path();
        path.addRoundRect(outBounds.getRectF(), outBounds.getBorderRadii(), Path.Direction.CW);
        canvas.clipPath(path, Region.Op.INTERSECT);
      }

      if (innerBounds != null) {
        Path path = new Path();
        path.addRoundRect(innerBounds.getRectF(), innerBounds.getBorderRadii(), Path.Direction.CW);
        canvas.clipPath(path, Region.Op.DIFFERENCE);
      }
    }
    clipQuadrilateral(canvas, x1, y1, x2, y2, x3, y3, x4, y4);
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
