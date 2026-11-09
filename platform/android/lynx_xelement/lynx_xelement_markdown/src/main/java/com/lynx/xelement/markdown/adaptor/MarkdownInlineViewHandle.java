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

  public MarkdownInlineViewHandle(
      NativeLayoutNodeRef layoutNodeRef, @Nullable LynxBaseUI ui, MarkdownResourceContext host) {
    mLayoutNodeRef = layoutNodeRef;
    mUI = ui;
    mHost = host;
  }

  @Override
  public void requestDraw() {}

  boolean isForNode(NativeLayoutNodeRef node) {
    return mLayoutNodeRef == node;
  }

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
    int widthResult = (int) result.getWidthResult();
    int heightResult = (int) result.getHeightResult();
    int baseline = (int) result.getBaselineResult();
    return MarkdownValuePack.packMeasureResult(widthResult, heightResult, baseline);
  }

  @Override
  public void align(int left, int top) {
    if (mLayoutNodeRef.isDestroyed()) {
      return;
    }
    AlignParam alignParam = new AlignParam();
    // Native alignment uses the markdown node's content box as its coordinate origin, so Lynx
    // applies the parent's padding and border while converting this position to the UI frame.
    alignParam.setLeftOffset(left);
    alignParam.setTopOffset(top);
    AlignContext alignContext = mHost.getAlignContext();
    if (alignContext == null) {
      alignContext = new AlignContext();
    }
    mLayoutNodeRef.alignNativeNode(alignContext, alignParam);
  }

  @Override
  public void setBounds(int left, int top, int width, int height) {
    LynxBaseUI ui = getUI();
    if (ui != null) {
      ui.setWidth(width);
      ui.setHeight(height);
      ui.setLeft(left + mHost.getContentLeftOffset());
      ui.setTop(top + mHost.getContentTopOffset());
    }
  }

  @Override
  public void setVisibility(boolean visible) {
    LynxBaseUI ui = getUI();
    if (ui instanceof LynxUI) {
      ((LynxUI<?>) ui).setVisibilityForView(visible ? View.VISIBLE : View.INVISIBLE);
    }
  }

  @Override
  public int getVerticalAlign() {
    ShadowStyle shadowStyle = mLayoutNodeRef.getShadowStyle();
    if (shadowStyle == null) {
      return Constants.VERTICAL_ALIGN_BASELINE;
    }
    switch (shadowStyle.verticalAlign) {
      case StyleConstants.VERTICAL_ALIGN_TOP:
        return Constants.VERTICAL_ALIGN_TOP;
      case StyleConstants.VERTICAL_ALIGN_TEXT_TOP:
        return Constants.VERTICAL_ALIGN_TEXT_TOP;
      case StyleConstants.VERTICAL_ALIGN_CENTER:
      case StyleConstants.VERTICAL_ALIGN_MIDDLE:
        return Constants.VERTICAL_ALIGN_CENTER;
      case StyleConstants.VERTICAL_ALIGN_BOTTOM:
        return Constants.VERTICAL_ALIGN_BOTTOM;
      case StyleConstants.VERTICAL_ALIGN_TEXT_BOTTOM:
        return Constants.VERTICAL_ALIGN_TEXT_BOTTOM;
      case StyleConstants.VERTICAL_ALIGN_LENGTH:
        return Constants.VERTICAL_ALIGN_LENGTH;
      default:
        return Constants.VERTICAL_ALIGN_BASELINE;
    }
  }

  @Override
  public float getVerticalAlignLength() {
    ShadowStyle shadowStyle = mLayoutNodeRef.getShadowStyle();
    return shadowStyle != null && shadowStyle.verticalAlign == StyleConstants.VERTICAL_ALIGN_LENGTH
        ? shadowStyle.verticalAlignLength
        : 0;
  }

  private @Nullable LynxBaseUI getUI() {
    LynxContext context = mHost.getLynxContext();
    if (mUI == null && context != null && context.getLynxUIOwner() != null) {
      mUI = context.getLynxUIOwner().findLynxUIBySign(mLayoutNodeRef.getSignature());
    }
    return mUI;
  }
}
