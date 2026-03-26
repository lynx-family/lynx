// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.view.View;
import androidx.annotation.Nullable;
import com.lynx.markdown.Constants;
import com.lynx.markdown.IMarkdownViewHandle;
import com.lynx.markdown.MarkdownValuePack;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.AlignContext;
import com.lynx.tasm.behavior.shadow.AlignParam;
import com.lynx.tasm.behavior.shadow.MeasureContext;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.MeasureParam;
import com.lynx.tasm.behavior.shadow.MeasureResult;
import com.lynx.tasm.behavior.shadow.NativeLayoutNodeRef;
import com.lynx.tasm.behavior.shadow.ShadowStyle;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;

public class MarkdownInlineViewHandle implements IMarkdownViewHandle {
  private final NativeLayoutNodeRef mLayoutNodeRef;
  private final MarkdownResourceContext mHost;
  private @Nullable LynxBaseUI mUI;
  private int mWidth = 0;
  private int mHeight = 0;
  private int mLeft = 0;
  private int mTop = 0;

  public MarkdownInlineViewHandle(
      NativeLayoutNodeRef layoutNodeRef, @Nullable LynxBaseUI ui, MarkdownResourceContext host) {
    mLayoutNodeRef = layoutNodeRef;
    mUI = ui;
    mHost = host;
  }

  @Override
  public void requestMeasure() {}

  @Override
  public void requestAlign() {}

  @Override
  public void requestDraw() {}

  @Override
  public long measure(int width, int widthMode, int height, int heightMode) {
    MeasureContext measureContext = mHost.getMeasureContext();
    if (mLayoutNodeRef.isDestroyed() || measureContext == null) {
      return MarkdownValuePack.packMeasureResult(0, 0, 0);
    }
    MeasureParam param = new MeasureParam();
    param.mWidth = width;
    param.mHeight = height;
    param.mWidthMode = MeasureMode.fromInt(widthMode);
    param.mHeightMode = MeasureMode.fromInt(heightMode);
    MeasureResult result = mLayoutNodeRef.measureNativeNodeWithBaseline(measureContext, param);
    mWidth = (int) result.getWidthResult();
    mHeight = (int) result.getHeightResult();
    int baseline = (int) result.getBaselineResult();
    return MarkdownValuePack.packMeasureResult(mWidth, mHeight, baseline);
  }

  @Override
  public void align(int left, int top) {
    if (mLayoutNodeRef.isDestroyed()) {
      return;
    }
    mLeft = left;
    mTop = top;
    AlignParam alignParam = new AlignParam();
    alignParam.setLeftOffset(left);
    alignParam.setTopOffset(top);
    AlignContext alignContext = mHost.getAlignContext();
    if (alignContext == null) {
      alignContext = new AlignContext();
    }
    mLayoutNodeRef.alignNativeNode(alignContext, alignParam);
  }

  @Override
  public long getSize() {
    LynxBaseUI ui = getUI();
    if (ui != null) {
      return MarkdownValuePack.packIntPair(ui.getWidth(), ui.getHeight());
    }
    return MarkdownValuePack.packIntPair(mWidth, mHeight);
  }

  @Override
  public long getPosition() {
    LynxBaseUI ui = getUI();
    if (ui != null) {
      return MarkdownValuePack.packIntPair(ui.getLeft(), ui.getTop());
    }
    return MarkdownValuePack.packIntPair(mLeft, mTop);
  }

  @Override
  public void setSize(int width, int height) {
    mWidth = width;
    mHeight = height;
    LynxBaseUI ui = getUI();
    if (ui != null) {
      ui.setWidth(width);
      ui.setHeight(height);
    }
  }

  @Override
  public void setPosition(int left, int top) {
    mLeft = left;
    mTop = top;
    LynxBaseUI ui = getUI();
    if (ui != null) {
      ui.setLeft(left);
      ui.setTop(top);
    }
  }

  @Override
  public void setVisibility(boolean visible) {
    LynxBaseUI ui = getUI();
    if (ui instanceof LynxUI) {
      ((LynxUI<?>) ui).setVisibility(visible ? View.VISIBLE : View.INVISIBLE);
    }
  }

  @Override
  public int getVerticalAlign() {
    ShadowStyle shadowStyle = mLayoutNodeRef.getShadowStyle();
    if (shadowStyle == null) {
      return Constants.VERTICAL_ALIGN_BASELINE;
    }
    int verticalAlign = shadowStyle.verticalAlign;
    if (verticalAlign == StyleConstants.VERTICAL_ALIGN_TOP
        || verticalAlign == StyleConstants.VERTICAL_ALIGN_TEXT_TOP) {
      return Constants.VERTICAL_ALIGN_TOP;
    }
    if (verticalAlign == StyleConstants.VERTICAL_ALIGN_CENTER
        || verticalAlign == StyleConstants.VERTICAL_ALIGN_MIDDLE) {
      return Constants.VERTICAL_ALIGN_CENTER;
    }
    if (verticalAlign == StyleConstants.VERTICAL_ALIGN_BOTTOM
        || verticalAlign == StyleConstants.VERTICAL_ALIGN_TEXT_BOTTOM) {
      return Constants.VERTICAL_ALIGN_BOTTOM;
    }
    return Constants.VERTICAL_ALIGN_BASELINE;
  }

  private @Nullable LynxBaseUI getUI() {
    LynxContext context = mHost.getLynxContext();
    if (mUI == null && context != null && context.getLynxUIOwner() != null) {
      mUI = context.getLynxUIOwner().findLynxUIBySign(mLayoutNodeRef.getSignature());
    }
    return mUI;
  }
}
