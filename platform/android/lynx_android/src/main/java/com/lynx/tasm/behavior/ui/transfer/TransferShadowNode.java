// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.transfer;

import com.lynx.tasm.behavior.shadow.AlignContext;
import com.lynx.tasm.behavior.shadow.AlignParam;
import com.lynx.tasm.behavior.shadow.CustomLayoutShadowNode;
import com.lynx.tasm.behavior.shadow.CustomMeasureFunc;
import com.lynx.tasm.behavior.shadow.MeasureContext;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.MeasureParam;
import com.lynx.tasm.behavior.shadow.MeasureResult;
import com.lynx.tasm.behavior.shadow.NativeLayoutNodeRef;
import com.lynx.tasm.behavior.shadow.ShadowNode;

public class TransferShadowNode extends CustomLayoutShadowNode implements CustomMeasureFunc {
  private boolean mHasHostConstraints;
  private float mHostWidth;
  private MeasureMode mHostWidthMode = MeasureMode.UNDEFINED;
  private float mHostHeight;
  private MeasureMode mHostHeightMode = MeasureMode.UNDEFINED;

  @Override
  public void attachNativePtr(long ptr) {
    setCustomMeasureFunc(this);
    super.attachNativePtr(ptr);
  }

  public void updateHostConstraints(
      float width, MeasureMode widthMode, float height, MeasureMode heightMode) {
    if (mHasHostConstraints && Float.compare(mHostWidth, width) == 0 && mHostWidthMode == widthMode
        && Float.compare(mHostHeight, height) == 0 && mHostHeightMode == heightMode) {
      return;
    }
    mHasHostConstraints = true;
    mHostWidth = width;
    mHostWidthMode = widthMode;
    mHostHeight = height;
    mHostHeightMode = heightMode;
    markDirty();
  }

  @Override
  public MeasureResult measure(MeasureParam param, MeasureContext context) {
    if (!mHasHostConstraints) {
      return new MeasureResult(0.0f, 0.0f);
    }
    MeasureParam childParam = new MeasureParam();
    childParam.mWidth = mHostWidthMode == MeasureMode.UNDEFINED ? param.mWidth : mHostWidth;
    childParam.mWidthMode =
        mHostWidthMode == MeasureMode.UNDEFINED ? param.mWidthMode : mHostWidthMode;
    childParam.mHeight = mHostHeightMode == MeasureMode.UNDEFINED ? param.mHeight : mHostHeight;
    childParam.mHeightMode =
        mHostHeightMode == MeasureMode.UNDEFINED ? param.mHeightMode : mHostHeightMode;
    for (int i = 0; i < getChildCount(); i++) {
      ShadowNode child = getChildAt(i);
      if (child instanceof NativeLayoutNodeRef) {
        ((NativeLayoutNodeRef) child).measureNativeNode(context, childParam);
      }
    }
    return new MeasureResult(0.0f, 0.0f);
  }

  @Override
  public void align(AlignParam param, AlignContext context) {
    for (int i = 0; i < getChildCount(); i++) {
      ShadowNode child = getChildAt(i);
      if (child instanceof NativeLayoutNodeRef) {
        ((NativeLayoutNodeRef) child).alignNativeNode(context, new AlignParam());
      }
    }
  }
}
