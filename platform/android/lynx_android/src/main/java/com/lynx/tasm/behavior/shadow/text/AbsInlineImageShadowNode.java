// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxPropGroup;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.shadow.ShadowStyle;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.text.AbsInlineImageSpan;
import com.lynx.tasm.behavior.ui.utils.BorderRadius;
import com.lynx.tasm.behavior.ui.utils.LynxBackground;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.utils.UnitUtils;
import java.util.List;
import java.util.Map;

public abstract class AbsInlineImageShadowNode extends BaseTextShadowNode {
  public static final String TAG_NAME = "inline-image";
  public static final String EVENT_ERROR = "error";
  public static final String EVENT_LOAD = "load";
  private boolean mBindLoad;
  private boolean mBindError;
  protected int mBackgroundColor = 0;
  private LynxBackground mComplexBackground;
  public AbsInlineImageShadowNode() {}

  @Override
  public void setEvents(Map<String, EventsListener> events) {
    super.setEvents(events);
    if (events != null) {
      mBindLoad = events.containsKey(EVENT_LOAD);
      mBindError = events.containsKey(EVENT_ERROR);
    }
  }

  protected void notifyLoadSuccessIfNeeded(int width, int height) {
    if (!mBindLoad) {
      return;
    }
    LynxDetailEvent event = new LynxDetailEvent(getSignature(), EVENT_LOAD);
    event.addDetail("height", height);
    event.addDetail("width", width);
    getContext().getEventEmitter().sendCustomEvent(event);
  }

  protected void notifyErrorIfNeeded(String errorMsg) {
    if (!mBindError) {
      return;
    }
    LynxDetailEvent event = new LynxDetailEvent(getSignature(), EVENT_ERROR);
    event.addDetail("errMsg", errorMsg);
    getContext().getEventEmitter().sendCustomEvent(event);
  }

  @LynxProp(name = PropsConstants.SRC) public abstract void setSource(@Nullable String source);

  @LynxProp(name = PropsConstants.MODE) public abstract void setMode(String mode);

  @LynxProp(name = PropsConstants.BACKGROUND_COLOR, defaultInt = 0)
  public void setBackgroundColor(int bgColor) {
    mBackgroundColor = bgColor;
  }

  @LynxProp(name = PropsConstants.VERTICAL_ALIGN)
  public void setVerticalAlign(@Nullable ReadableArray array) {
    setVerticalAlignOnShadowNode(array);
  }

  private LynxBackground getOrCreateComplexBackground() {
    if (mComplexBackground == null) {
      mComplexBackground = new LynxBackground(getContext());
    }
    return mComplexBackground;
  }

  @LynxPropGroup(names =
                     {
                         PropsConstants.BORDER_RADIUS,
                         PropsConstants.BORDER_TOP_LEFT_RADIUS,
                         PropsConstants.BORDER_TOP_RIGHT_RADIUS,
                         PropsConstants.BORDER_BOTTOM_RIGHT_RADIUS,
                         PropsConstants.BORDER_BOTTOM_LEFT_RADIUS,
                     })
  public void
  setBorderRadius(int index, @Nullable ReadableArray ra) {
    getOrCreateComplexBackground().setBorderRadius(index, ra);
  }

  @LynxPropGroup(names =
                     {
                         PropsConstants.BORDER_STYLE,
                         PropsConstants.BORDER_LEFT_STYLE,
                         PropsConstants.BORDER_RIGHT_STYLE,
                         PropsConstants.BORDER_TOP_STYLE,
                         PropsConstants.BORDER_BOTTOM_STYLE,
                     },
      defaultInt = -1)
  public void
  setBorderStyle(int index, int borderStyle) {
    getOrCreateComplexBackground().setBorderStyle(LynxBaseUI.SPACING_TYPES[index], borderStyle);
  }

  @LynxPropGroup(names =
                     {
                         PropsConstants.BORDER_WIDTH,
                         PropsConstants.BORDER_LEFT_WIDTH,
                         PropsConstants.BORDER_RIGHT_WIDTH,
                         PropsConstants.BORDER_TOP_WIDTH,
                         PropsConstants.BORDER_BOTTOM_WIDTH,
                     })
  public void
  setBorderWidth(int index, int borderWidth) {
    getOrCreateComplexBackground().setBorderWidth(LynxBaseUI.SPACING_TYPES[index], borderWidth);
  }

  @LynxPropGroup(names =
                     {
                         PropsConstants.BORDER_LEFT_COLOR,
                         PropsConstants.BORDER_RIGHT_COLOR,
                         PropsConstants.BORDER_TOP_COLOR,
                         PropsConstants.BORDER_BOTTOM_COLOR,
                     },
      customType = "Color")
  public void
  setBorderColor(int index, Integer color) {
    getOrCreateComplexBackground().setBorderColorForSpacingIndex(
        LynxBaseUI.SPACING_TYPES[index + 1], color);
  }

  @Override
  public boolean isVirtual() {
    return true;
  }

  public abstract AbsInlineImageSpan generateInlineImageSpan();

  protected void setSpanVerticalAlign(AbsInlineImageSpan span) {
    ShadowStyle shadowStyle = getShadowStyle();
    if (shadowStyle != null) {
      span.setVerticalAlign(shadowStyle.verticalAlign, shadowStyle.verticalAlignLength);
    }
  }

  protected void generateStyleSpan(
      int start, int end, List<BaseTextShadowNode.SetSpanOperation> ops) {
    AbsInlineImageSpan span = generateInlineImageSpan();
    setSpanVerticalAlign(span);
    if (mComplexBackground != null && mComplexBackground.getDrawable() != null) {
      mComplexBackground.getDrawable().setBounds(0, 0, (int) Math.ceil(getStyle().getWidth()),
          (int) Math.ceil((int) getStyle().getHeight()));
      mComplexBackground.setBackgroundColor(mBackgroundColor);
      span.setComplexBackground(mComplexBackground);
    } else {
      span.setBackgroundColor(mBackgroundColor);
    }
    span.setVerticalShift(getTextAttributes().mBaselineShift);
    ops.add(new BaseTextShadowNode.SetSpanOperation(start, end, span));
    if (needGenerateEventTargetSpan()) {
      ops.add(new BaseTextShadowNode.SetSpanOperation(start, end, toEventTargetSpan()));
    }
  }
}
