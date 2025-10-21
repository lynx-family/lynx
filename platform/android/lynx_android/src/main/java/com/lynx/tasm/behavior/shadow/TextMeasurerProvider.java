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
   * @return Array containing the measurement results [width, height]
   */
  float[] measureText(int sign, float width, int widthMode, float height, int heightMode);

  /**
   * Dispatches layout information before text layout.
   *
   * @param sign The sign identifier
   * @param buffer The layout information buffer
   */
  void dispatchLayoutBefore(int sign, ReadableCompactArrayBuffer buffer);
}
