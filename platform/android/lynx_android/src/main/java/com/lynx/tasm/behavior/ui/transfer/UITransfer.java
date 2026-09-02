// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.transfer;

import android.content.Context;
import android.view.View;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.UIGroup;

public class UITransfer extends UIGroup<TransferHostView> {
  private TransferWrapperView mWrapperView;
  private float mHostWidth;
  private MeasureMode mHostWidthMode = MeasureMode.UNDEFINED;
  private float mHostHeight;
  private MeasureMode mHostHeightMode = MeasureMode.UNDEFINED;

  public UITransfer(LynxContext context) {
    this(context, null);
  }

  public UITransfer(LynxContext context, Object params) {
    super(context, params);
  }

  @Override
  protected TransferHostView createView(Context context) {
    mWrapperView = new TransferWrapperView(context, this);
    return new TransferHostView(context, mWrapperView);
  }

  @Override
  public boolean canHaveFlattenChild() {
    return false;
  }

  @Override
  public boolean isExternalExposureRoot() {
    return true;
  }

  void updateHostConstraints(int widthMeasureSpec, int heightMeasureSpec) {
    MeasureMode widthMode = MeasureMode.fromInt(MeasureMode.fromMeasureSpec(widthMeasureSpec));
    MeasureMode heightMode = MeasureMode.fromInt(MeasureMode.fromMeasureSpec(heightMeasureSpec));
    float width =
        widthMode == MeasureMode.UNDEFINED ? 0.0f : View.MeasureSpec.getSize(widthMeasureSpec);
    float height =
        heightMode == MeasureMode.UNDEFINED ? 0.0f : View.MeasureSpec.getSize(heightMeasureSpec);
    if (Float.compare(mHostWidth, width) == 0 && mHostWidthMode == widthMode
        && Float.compare(mHostHeight, height) == 0 && mHostHeightMode == heightMode) {
      return;
    }
    mHostWidth = width;
    mHostWidthMode = widthMode;
    mHostHeight = height;
    mHostHeightMode = heightMode;
    mContext.findShadowNodeAndRunTask(getSign(), (ShadowNode node) -> {
      if (node instanceof TransferShadowNode) {
        ((TransferShadowNode) node)
            .updateHostConstraints(mHostWidth, mHostWidthMode, mHostHeight, mHostHeightMode);
      }
    });
  }
}
