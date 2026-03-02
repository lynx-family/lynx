// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

import com.lynx.react.bridge.mapbuffer.ReadableCompactArrayBuffer;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.LynxUIOwner;

public class TextLayout {
  final TextMeasurerProvider mTextMeasurerProvider;

  /**
   * Constructor using the decoupled TextMeasurerProvider interface.
   * This allows TextLayout to work with any implementation that provides text measurement.
   */
  public TextLayout(TextMeasurerProvider textMeasurerProvider) {
    mTextMeasurerProvider = textMeasurerProvider;
  }

  /**
   * Legacy constructor for backward compatibility with LynxUIOwner.
   * This constructor adapts LynxUIOwner to the TextMeasurerProvider interface.
   */
  public TextLayout(LynxUIOwner uiOwner) {
    mTextMeasurerProvider = new TextMeasurerProvider() {
      @Override
      public float[] measureText(int sign, float width, int widthMode, float height, int heightMode,
          float[] inlineViewLayoutResult) {
        return uiOwner.measureText(
            sign, width, widthMode, height, heightMode, inlineViewLayoutResult);
      }

      @Override
      public void dispatchLayoutBefore(int sign, ReadableCompactArrayBuffer buffer) {
        uiOwner.dispatchLayoutBefore(sign, buffer);
      }

      @Override
      public float[] align(int sign) {
        return uiOwner.align(sign);
      }
    };
  }

  @CalledByNative
  public float[] measureText(int sign, float width, int widthMode, float height, int heightMode,
      float[] inlineViewLayoutResult) {
    TraceEvent.beginSection(TraceEventDef.TEXT_LAYOUT_MEASURE_TEXT);
    float[] result = mTextMeasurerProvider.measureText(
        sign, width, widthMode, height, heightMode, inlineViewLayoutResult);
    TraceEvent.endSection(TraceEventDef.TEXT_LAYOUT_MEASURE_TEXT);
    return result;
  }

  @CalledByNative
  public void dispatchLayoutBefore(int sign, ReadableCompactArrayBuffer buffer) {
    TraceEvent.beginSection(TraceEventDef.TEXT_LAYOUT_DISPATCH_LAYOUT_BEFORE);
    mTextMeasurerProvider.dispatchLayoutBefore(sign, buffer);
    TraceEvent.endSection(TraceEventDef.TEXT_LAYOUT_DISPATCH_LAYOUT_BEFORE);
  }

  @CalledByNative
  public float[] align(int sign) {
    TraceEvent.beginSection(TraceEventDef.TEXT_LAYOUT_ALIGN);
    float[] res = mTextMeasurerProvider.align(sign);
    TraceEvent.endSection(TraceEventDef.TEXT_LAYOUT_ALIGN);
    return res;
  }
}
