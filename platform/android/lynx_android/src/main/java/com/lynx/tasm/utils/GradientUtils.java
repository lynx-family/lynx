// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import android.graphics.PointF;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.mapbuffer.MapBuffer;
import com.lynx.react.bridge.mapbuffer.ReadableMapBuffer;
import com.lynx.react.bridge.mapbuffer.ReadableMapBufferWrapper;
import java.util.Iterator;

public class GradientUtils {
  public static PointF getRadius(int shape, int shapeSize, float cx, float cy, float sx, float sy) {
    float[] radius = nativeGetRadialRadius(shape, shapeSize, cx, cy, sx, sy);
    return new PointF(radius[0], radius[1]);
  }

  public static ReadableArray getGradientArray(String gradientDef, float screen_width,
      float layouts_unit_per_px, float physical_pixels_per_layout_unit, float root_node_font_size,
      float cur_node_font_size, float font_scale, float viewport_width, float viewport_height) {
    MapBuffer buffer = nativeGetGradientArray(gradientDef, screen_width, layouts_unit_per_px,
        physical_pixels_per_layout_unit, root_node_font_size, cur_node_font_size, font_scale,
        viewport_width, viewport_height);
    if (buffer == null) {
      return null;
    }
    Iterator<MapBuffer.Entry> iterator = buffer.iterator();
    if (iterator.hasNext()) {
      return new ReadableMapBufferWrapper(iterator.next().getMapBuffer());
    }
    return null;
  }

  private static native float[] nativeGetRadialRadius(
      int shape, int shapeSize, float cx, float cy, float sx, float sy);

  private static native ReadableMapBuffer nativeGetGradientArray(String gradientDef,
      float screen_width, float layouts_unit_per_px, float physical_pixels_per_layout_unit,
      float root_node_font_size, float cur_node_font_size, float font_scale, float viewport_width,
      float viewport_height);

  /**
   * Calculates the start and end points for a linear gradient line using a CSS angle.
   * This method implements the distance-based algorithm for consistent cross-platform behavior.
   *
   * @param angleDegrees The CSS gradient angle in degrees (0deg = to top, 90deg = to right)
   * @param width The width of the gradient box
   * @param height The height of the gradient box
   * @param start Output point for the calculated start point (in box coordinates, origin at center)
   * @param end Output point for the calculated end point (in box coordinates, origin at center)
   */
  public static void calculateGradientLine(
      float angleDegrees, float width, float height, PointF start, PointF end) {
    // Convert CSS angle to standard math angle
    // CSS: 0deg = to top, 90deg = to right
    // Math: 0 = right, 90 = top (counter-clockwise from right)
    double angleRad = Math.toRadians(angleDegrees - 90.0);

    float centerX = width / 2.0f;
    float centerY = height / 2.0f;

    double sinVal = Math.sin(angleRad);
    double cosVal = Math.cos(angleRad);

    // Handle edge cases where cos or sin is 0 (horizontal/vertical gradients)
    double dx = (cosVal == 0) ? Double.POSITIVE_INFINITY : Math.abs(width / 2.0 / cosVal);
    double dy = (sinVal == 0) ? Double.POSITIVE_INFINITY : Math.abs(height / 2.0 / sinVal);

    double distance;
    if (Double.isInfinite(dx) || Double.isNaN(dx)) {
      distance = dy;
    } else if (Double.isInfinite(dy) || Double.isNaN(dy)) {
      distance = dx;
    } else {
      distance = Math.min(dx, dy);
    }

    // Fallback if both are invalid
    if (Double.isInfinite(distance) || Double.isNaN(distance)) {
      distance = Math.max(width, height);
    }

    start.x = (float) (centerX - distance * cosVal);
    start.y = (float) (centerY - distance * sinVal);
    end.x = (float) (centerX + distance * cosVal);
    end.y = (float) (centerY + distance * sinVal);
  }
}
