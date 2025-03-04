// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

import com.lynx.tasm.behavior.LynxProp;

public class CustomLayoutShadowNode extends ShadowNode {
  public boolean mCustomLayout = false;

  @Override
  public boolean supportInlineView() {
    return true;
  }

  @LynxProp(name = "custom-layout")
  public void setCustomLayout(boolean customLayout) {
    mCustomLayout = customLayout;
  }

  public MeasureResult measureNativeNode(MeasureContext context, MeasureParam param) {
    long result = layoutNodeManager.measureNativeNode(getSignature(), param.mWidth,
        param.mWidthMode.intValue(), param.mHeight, param.mHeightMode.intValue(),
        context.mFinalMeasure);
    float resultWidth = MeasureOutput.getWidth(result);
    float resultHeight = MeasureOutput.getHeight(result);
    return new MeasureResult(resultWidth, resultHeight);
  }

  public void alignNativeNode(AlignContext context, AlignParam param) {
    layoutNodeManager.alignNativeNode(getSignature(), param.getTopOffset(), param.getLeftOffset());
  }
}
