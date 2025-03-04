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
}
