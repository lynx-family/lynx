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
  final LynxUIOwner mUIOwner;
  public TextLayout(LynxUIOwner uiOwner) {
    mUIOwner = uiOwner;
  }

  @CalledByNative
  public float[] measureText(int sign, float width, int widthMode, float height, int heightMode) {
    TraceEvent.beginSection(TraceEventDef.TEXT_LAYOUT_MEASURE_TEXT);
    float[] result = mUIOwner.measureText(sign, width, widthMode, height, heightMode);
    TraceEvent.endSection(TraceEventDef.TEXT_LAYOUT_MEASURE_TEXT);
    return result;
  }
  @CalledByNative
  public void dispatchLayoutBefore(int sign, ReadableCompactArrayBuffer buffer) {
    TraceEvent.beginSection(TraceEventDef.TEXT_LAYOUT_DISPATCH_LAYOUT_BEFORE);
    mUIOwner.dispatchLayoutBefore(sign, buffer);
    TraceEvent.endSection(TraceEventDef.TEXT_LAYOUT_DISPATCH_LAYOUT_BEFORE);
  }

  @CalledByNative
  public void align() {}
}
