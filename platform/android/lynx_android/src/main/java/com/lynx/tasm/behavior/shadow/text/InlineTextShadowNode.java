// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Typeface;
import android.text.TextUtils;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxPropGroup;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.utils.LynxBackground;
import com.lynx.tasm.fontface.FontFaceManager;
import com.lynx.tasm.utils.DeviceUtils;
import java.util.List;

public class InlineTextShadowNode extends BaseTextShadowNode {
  private static final String TAG = "InlineTextShadowNode";
  private int mBackgroundColor = 0;
  private LynxBackground mBackground;
  private boolean mIsSetBackGroundImage = false;

  @Override
  public void setContext(LynxContext context) {
    super.setContext(context);
    mBackground = new LynxBackground(context);
  }

  @SuppressWarnings("DefaultAnnotationParam")
  @LynxProp(name = PropsConstants.BACKGROUND_COLOR, defaultInt = 0)
  public void setBackgroundColor(int bgColor) {
    if (mBackgroundColor != bgColor) {
      mBackgroundColor = bgColor;
      markDirty();
    }
  }

  @LynxProp(name = PropsConstants.VERTICAL_ALIGN)
  public void setVerticalAlign(@Nullable ReadableArray array) {
    setVerticalAlignOnShadowNode(array);
  }

  @LynxProp(name = PropsConstants.BACKGROUND_IMAGE)
  public void setBackgroundImage(@Nullable ReadableArray bgImage) {
    markDirty();
    if (bgImage == null) {
      mIsSetBackGroundImage = false;
      return;
    }
    mBackground.setLayerImage(bgImage, null);
    mIsSetBackGroundImage = true;
  }

  @LynxProp(name = PropsConstants.BACKGROUND_SIZE)
  public void setBackgroundSize(@Nullable ReadableArray bgSize) {
    mBackground.setLayerSize(bgSize);
    markDirty();
  }

  @LynxProp(name = PropsConstants.BACKGROUND_REPEAT)
  public void setBackgroundRepeat(@Nullable ReadableArray bgImgRepeat) {
    mBackground.setLayerRepeat(bgImgRepeat);
    markDirty();
  }

  @LynxProp(name = PropsConstants.BACKGROUND_POSITION)
  public void setBackgroundPosition(@Nullable ReadableArray bgImgPosition) {
    mBackground.setLayerPosition(bgImgPosition);
    markDirty();
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
    mBackground.setBorderRadius(index, ra);
    markDirty();
  }

  @Override
  protected void buildStyledSpan(int start, int end, List<SetSpanOperation> ops) {
    super.buildStyledSpan(start, end, ops);
    if (needGenerateEventTargetSpan()) {
      ops.add(new SetSpanOperation(start, end, toEventTargetSpan()));
    }

    if (mBackgroundColor != 0) {
      ops.add(new SetSpanOperation(start, end, new BackgroundColorSpan(mBackgroundColor)));
    }

    if (mIsSetBackGroundImage) {
      ops.add(
          new SetSpanOperation(start, end, new LynxTextBackgroundSpan(start, end, mBackground)));
    }

    if (getTextAttributes().mFontSize != MeasureUtils.UNDEFINED) {
      ops.add(new SetSpanOperation(
          start, end, new AbsoluteSizeSpan(Math.round(getTextAttributes().mFontSize))));
    }
    // Set font family
    if (!TextUtils.isEmpty(getTextAttributes().mFontFamily)) {
      String fontFamily = getTextAttributes().mFontFamily;
      int style = getTypefaceStyle();
      Typeface typeface = TypefaceCache.getTypeface(getContext(), fontFamily, style);
      if (typeface == null) {
        FontFaceManager.getInstance().getTypeface(
            getContext(), fontFamily, style, new TextShadowNode.WeakTypefaceListener(this));
        // If typeface is null, avoid setting typeface, see TextHelper.newTextPaint
        typeface = DeviceUtils.getDefaultTypeface();
      }
      ops.add(new SetSpanOperation(start, end, new FontFamilySpan(typeface)));
    }
  }

  @Override
  public void setTextAlign(int textAlign) {
    super.setTextAlign(textAlign);
    if (isTextRefactorEnabled()) {
      LLog.e(
          TAG, "inline-text will no longer support text-align in future, set on root text instead");
    }
  }

  @Override
  public boolean isVirtual() {
    return true;
  }
}
