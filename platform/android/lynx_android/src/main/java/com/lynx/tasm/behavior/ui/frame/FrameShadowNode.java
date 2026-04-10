// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.LynxGeneratorName;
import com.lynx.tasm.behavior.LynxShadowNode;
import com.lynx.tasm.behavior.shadow.AlignContext;
import com.lynx.tasm.behavior.shadow.AlignParam;
import com.lynx.tasm.behavior.shadow.CustomLayoutShadowNode;
import com.lynx.tasm.behavior.shadow.CustomMeasureFunc;
import com.lynx.tasm.behavior.shadow.MeasureContext;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.MeasureParam;
import com.lynx.tasm.behavior.shadow.MeasureResult;
import java.util.HashMap;

@LynxGeneratorName(packageName = "com.lynx.tasm.behavior.ui.frame")
@LynxShadowNode(tagName = "frame")
public class FrameShadowNode extends CustomLayoutShadowNode implements CustomMeasureFunc {
  private static final String TAG = "FrameShadowNode";
  private int mIntrinsicWidth;
  private int mIntrinsicHeight;

  @Override
  public void attachNativePtr(long ptr) {
    setCustomMeasureFunc(this);
    super.attachNativePtr(ptr);
  }

  /**
   * @brief Update the intrinsic content size of the FrameShadowNode.
   * @param width The intrinsic width of the FrameShadowNode.
   * @param height The intrinsic height of the FrameShadowNode.
   */
  public void updateIntrinsicContentSize(int width, int height) {
    if (TraceEvent.isTracingStarted()) {
      HashMap<String, String> traceProps = new HashMap<>();
      traceProps.put("sign", String.valueOf(getSignature()));
      traceProps.put("old_width", String.valueOf(mIntrinsicWidth));
      traceProps.put("old_height", String.valueOf(mIntrinsicHeight));
      traceProps.put("new_width", String.valueOf(width));
      traceProps.put("new_height", String.valueOf(height));
      TraceEvent.instant(TraceEvent.CATEGORY_DEFAULT,
          "FrameIntrinsicTrace.FrameShadowNode.updateIntrinsicContentSize", traceProps);
    }
    if (width != mIntrinsicWidth || height != mIntrinsicHeight) {
      mIntrinsicWidth = width;
      mIntrinsicHeight = height;
      markDirty();
    }
  }

  @Override
  public MeasureResult measure(MeasureParam param, MeasureContext context) {
    if (TraceEvent.isTracingStarted()) {
      HashMap<String, String> args = new HashMap<>();
      args.put("input_width", String.valueOf(param.mWidth));
      args.put("input_height", String.valueOf(param.mHeight));
      args.put("input_width_mode", param.mWidthMode.name());
      args.put("input_height_mode", param.mHeightMode.name());
      TraceEvent.beginSection(TraceEventDef.FRAME_SHADOW_NODE_MEASURE, args);
    }
    MeasureResult result;
    if (mIntrinsicWidth == 0 && mIntrinsicHeight == 0) {
      result = new MeasureResult(param.mWidth, param.mHeight);
    } else {
      float width = param.mWidthMode == MeasureMode.EXACTLY ? param.mWidth : mIntrinsicWidth;
      float height = param.mHeightMode == MeasureMode.EXACTLY ? param.mHeight : mIntrinsicHeight;
      result = new MeasureResult(width, height);
    }
    if (TraceEvent.isTracingStarted()) {
      HashMap<String, String> args = new HashMap<>();
      args.put("width", String.valueOf(result.getWidthResult()));
      args.put("height", String.valueOf(result.getHeightResult()));
      TraceEvent.endSection(
          TraceEvent.CATEGORY_DEFAULT, TraceEventDef.FRAME_SHADOW_NODE_MEASURE, args);
    }
    return result;
  }

  @Override
  public void align(AlignParam param, AlignContext context) {}
}
