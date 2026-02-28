// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

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
    if (width != mIntrinsicWidth || height != mIntrinsicHeight) {
      mIntrinsicWidth = width;
      mIntrinsicHeight = height;
      markDirty();
    }
  }

  @Override
  public MeasureResult measure(MeasureParam param, MeasureContext context) {
    if (mIntrinsicWidth == 0 && mIntrinsicHeight == 0) {
      return new MeasureResult(param.mWidth, param.mHeight);
    }
    float width = param.mWidthMode == MeasureMode.EXACTLY ? param.mWidth : mIntrinsicWidth;
    float height = param.mHeightMode == MeasureMode.EXACTLY ? param.mHeight : mIntrinsicHeight;
    return new MeasureResult(width, height);
  }

  @Override
  public void align(AlignParam param, AlignContext context) {}
}
