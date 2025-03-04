// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.shadow.text.BaseTextShadowNode;
import com.lynx.tasm.behavior.shadow.text.EventTargetSpan;
import com.lynx.tasm.behavior.shadow.text.NativeLayoutNodeSpan;
import com.lynx.tasm.event.EventsListener;
import java.util.List;
import java.util.Map;

public class NativeLayoutNodeRef extends ShadowNode {
  public NativeLayoutNodeRef() {}

  // actual index after layout
  private int mSpanStart;
  private int mSpanEnd;

  // save origin index in truncation string, reset the index before relayout
  private int mSpanOriginStart;
  private int mSpanOriginEnd;

  private String mIdSelector;

  public void updateNativeNodeIndex(int moveOffset) {
    mSpanStart += moveOffset;
    mSpanEnd += moveOffset;
  }

  public void resetNativeNodeIndex() {
    mSpanStart = mSpanOriginStart;
    mSpanEnd = mSpanOriginEnd;
  }

  public int getSpanStart() {
    return mSpanStart;
  }

  public int getSpanEnd() {
    return mSpanEnd;
  }

  @LynxProp(name = PropsConstants.VERTICAL_ALIGN)
  public void setVerticalAlign(@Nullable ReadableArray array) {
    setVerticalAlignOnShadowNode(array);
  }

  @LynxProp(name = PropsConstants.ID_SELECTOR)
  public void setIdSelector(String id) {
    mIdSelector = id;
  }
  public String getIdSelector() {
    return mIdSelector;
  }

  @Override
  public void setContext(LynxContext context) {
    super.setContext(context);
  }

  // native node's EventTargetSpan is an virtual span.
  public static class InlineViewEventSpan extends EventTargetSpan {
    public InlineViewEventSpan(int sign, Map<String, EventsListener> events,
        EventTarget.EnableStatus ignoreFocus, boolean enableTouchPseudoPropagation,
        EventTarget.EnableStatus eventThrough, ReadableMap dataset) {
      super(sign, events, ignoreFocus, enableTouchPseudoPropagation, eventThrough, dataset);
    }
  }

  @Override
  public boolean needGenerateEventTargetSpan() {
    // always return true because there is no way to know whether an event is bound on the inline
    // view
    return true;
  }

  @Override
  public EventTargetSpan toEventTargetSpan() {
    return new InlineViewEventSpan(getSignature(), mEvents, mIgnoreFocus,
        mEnableTouchPseudoPropagation, mEventThrough, mDataset);
  }

  public NativeLayoutNodeSpan generateStyleSpan(
      int start, int end, List<BaseTextShadowNode.SetSpanOperation> ops) {
    mSpanStart = mSpanOriginStart = start;
    mSpanEnd = mSpanOriginEnd = end;
    NativeLayoutNodeSpan span = new NativeLayoutNodeSpan();
    if (getShadowStyle() != null) {
      span.setVerticalAlign(getShadowStyle().verticalAlign, getShadowStyle().verticalAlignLength);
    }
    ops.add(new BaseTextShadowNode.SetSpanOperation(start, end, span));
    if (needGenerateEventTargetSpan()) {
      ops.add(new BaseTextShadowNode.SetSpanOperation(start, end, toEventTargetSpan()));
    }
    return span;
  }

  public MeasureResult measureNativeNode(MeasureContext context, MeasureParam param) {
    if (isDestroyed()) {
      reportNullError("measureNativeNode for null, tag: " + getTagName());
      return new MeasureResult(0, 0);
    }
    if (getShadowStyle() != null
        && getShadowStyle().verticalAlign == StyleConstants.VERTICAL_ALIGN_BASELINE) {
      int[] result = layoutNodeManager.measureNativeNodeReturnWithBaseline(getSignature(),
          param.mWidth, param.mWidthMode.intValue(), param.mHeight, param.mHeightMode.intValue(),
          context.mFinalMeasure);
      return new MeasureResult(result[0], result[1], result[2]);
    } else {
      long result = layoutNodeManager.measureNativeNode(getSignature(), param.mWidth,
          param.mWidthMode.intValue(), param.mHeight, param.mHeightMode.intValue(),
          context.mFinalMeasure);
      float resultWidth = MeasureOutput.getWidth(result);
      float resultHeight = MeasureOutput.getHeight(result);
      return new MeasureResult(resultWidth, resultHeight);
    }
  }

  public MeasureResult measureNativeNodeWithBaseline(MeasureContext context, MeasureParam param) {
    if (isDestroyed()) {
      reportNullError("measureNativeNode for null, tag: " + getTagName());
      return new MeasureResult(0, 0);
    }
    int[] result = layoutNodeManager.measureNativeNodeReturnWithBaseline(getSignature(),
        param.mWidth, param.mWidthMode.intValue(), param.mHeight, param.mHeightMode.intValue(),
        context.mFinalMeasure);
    return new MeasureResult(result[0], result[1], result[2]);
  }

  public void alignNativeNode(AlignContext context, AlignParam param) {
    if (isDestroyed()) {
      reportNullError("alignNativeNode for null, tag: " + getTagName());
      return;
    }
    layoutNodeManager.alignNativeNode(getSignature(), param.getTopOffset(), param.getLeftOffset());
  }

  @Override
  public boolean supportInlineView() {
    return true;
  }
}
