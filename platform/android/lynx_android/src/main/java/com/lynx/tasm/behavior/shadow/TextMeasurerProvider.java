// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

import com.lynx.react.bridge.mapbuffer.ReadableCompactArrayBuffer;

/**
 * Interface for providing text measurement functionality.
 * This decouples TextLayout from the concrete LynxUIOwner implementation.
 */
public interface TextMeasurerProvider {
  /**
   * Measures text with the given constraints.
   *
   * @param sign The sign identifier for the text measurement
   * @param width The width constraint
   * @param widthMode The width measurement mode
   * @param height The height constraint
   * @param heightMode The height measurement mode
   * @param inlineViewLayoutResult The inline view layout result array
   * @return Array containing the measurement results [width, height]
   */
  float[] measureText(int sign, float width, int widthMode, float height, int heightMode,
      float[] inlineViewLayoutResult);

  /**
   * Dispatches layout information before text layout.
   *
   * @param sign The sign identifier
   * @param buffer The layout information buffer
   */
  void dispatchLayoutBefore(int sign, ReadableCompactArrayBuffer buffer);

  /**
   * Aligns text with the given sign.
   *
   * @param sign The sign identifier
   * @return Array containing children element's sign and alignment results [sign, left, top]
   */
  float[] align(int sign);
}
