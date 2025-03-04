// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.swiper;

import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.behavior.LynxGeneratorName;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxShadowNode;
import com.lynx.tasm.behavior.shadow.AlignContext;
import com.lynx.tasm.behavior.shadow.AlignParam;
import com.lynx.tasm.behavior.shadow.CustomLayoutShadowNode;
import com.lynx.tasm.behavior.shadow.CustomMeasureFunc;
import com.lynx.tasm.behavior.shadow.MeasureContext;
import com.lynx.tasm.behavior.shadow.MeasureParam;
import com.lynx.tasm.behavior.shadow.MeasureResult;
import com.lynx.tasm.behavior.shadow.NativeLayoutNodeRef;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.utils.UnitUtils;

@LynxGeneratorName(packageName = "com.lynx.tasm.behavior.ui.swiper")
@LynxShadowNode(tagName = "swiper")
public class SwiperShadowNode extends CustomLayoutShadowNode implements CustomMeasureFunc {
  private static final String MODE_DEFAULT = "normal";
  private static final String MODE_CAROUSEL = "carousel";
  private static final String MODE_COVER_FLOW = "coverflow";
  private static final String MODE_FLAT_COVER_FLOW = "flat-coverflow";
  private static final String MODE_CARRY = "carry";

  private int mPreviousMargin = -1;
  private int mNextMargin = -1;
  private int mPageMargin = -1;
  private float mXScale = 1;
  private float mYScale = 1;
  private boolean mIsVertical = false;
  private String mMode = MODE_DEFAULT;

  @Override
  public void attachNativePtr(long ptr) {
    if (mCustomLayout) {
      setCustomMeasureFunc(this);
    }
    super.attachNativePtr(ptr);
  }

  @LynxProp(name = "mode")
  public void setMode(String mode) {
    mMode = mode;
    if (mCustomLayout) {
      this.markDirty();
    }
  }

  @LynxProp(name = "previous-margin")
  public void setPreviousMargin(Dynamic previousMarginValue) {
    ReadableType type = previousMarginValue.getType();
    if (type != ReadableType.String) {
      return;
    }
    String previousMarginStrValue = previousMarginValue.asString();
    if (previousMarginStrValue.endsWith("px") || previousMarginStrValue.endsWith("rpx")) {
      int value = (int) UnitUtils.toPxWithDisplayMetrics(
          previousMarginStrValue, 0, -1.0f, mContext.getScreenMetrics());
      mPreviousMargin = value >= 0 ? value : -1;
    }
    if (mCustomLayout) {
      this.markDirty();
    }
  }

  @LynxProp(name = "next-margin")
  public void setNextMargin(Dynamic nextMarginValue) {
    ReadableType type = nextMarginValue.getType();
    if (type != ReadableType.String) {
      return;
    }
    String nextMarginStrValue = nextMarginValue.asString();
    if (nextMarginStrValue.endsWith("px") || nextMarginStrValue.endsWith("rpx")) {
      int value = (int) UnitUtils.toPxWithDisplayMetrics(
          nextMarginStrValue, 0, -1.0f, mContext.getScreenMetrics());
      mNextMargin = value >= 0 ? value : -1;
    }
    if (mCustomLayout) {
      this.markDirty();
    }
  }

  @LynxProp(name = "page-margin")
  public void setPageMargin(Dynamic pageMargin) {
    if (pageMargin.getType() == ReadableType.String) {
      String pageMarginStrValue = pageMargin.asString();
      if (pageMarginStrValue.endsWith("px") || pageMarginStrValue.endsWith("rpx")) {
        int margin = (int) UnitUtils.toPxWithDisplayMetrics(
            pageMarginStrValue, 0, 0.f, mContext.getScreenMetrics());
        mPageMargin = margin > 0 ? margin : 0;
      }
      if (mCustomLayout) {
        this.markDirty();
      }
    }
  }

  @LynxProp(name = "max-x-scale")
  public void setMaxXScale(double scale) {
    if (scale >= 0) {
      mXScale = (float) scale;
    }
    if (mCustomLayout) {
      this.markDirty();
    }
  }

  @LynxProp(name = "max-y-scale")
  public void setMaxYScale(double scale) {
    if (scale >= 0) {
      mYScale = (float) scale;
    }
    if (mCustomLayout) {
      this.markDirty();
    }
  }

  @LynxProp(name = "vertical", defaultBoolean = false)
  public void setVertical(boolean isVertical) {
    mIsVertical = isVertical;
    if (mCustomLayout) {
      this.markDirty();
    }
  }

  @Override
  public MeasureResult measure(MeasureParam param, MeasureContext context) {
    MeasureParam cParam = null;
    for (int i = 0; i < this.getChildCount(); i++) {
      ShadowNode node = this.getChildAt(i);
      if (node instanceof NativeLayoutNodeRef) {
        NativeLayoutNodeRef child = (NativeLayoutNodeRef) node;
        if (cParam != null) {
          child.measureNativeNode(context, cParam);
          continue;
        }
        cParam = new MeasureParam();
        if (mMode.equals(MODE_COVER_FLOW) || mMode.equals(MODE_FLAT_COVER_FLOW)) {
          float totalMargin = mPreviousMargin + mNextMargin + mPageMargin * 2;
          cParam.updateConstraints(param.mWidth - (mIsVertical ? 0 : totalMargin), param.mWidthMode,
              param.mHeight - (mIsVertical ? totalMargin : 0), param.mHeightMode);
        } else if (mMode.equals(MODE_CAROUSEL)) {
          float width = 0.f;
          float height = 0.f;
          if (mIsVertical) {
            height = (float) (param.mHeight * 0.8);
            width = param.mWidth;
          } else {
            height = param.mHeight;
            width = (float) (param.mWidth * 0.8);
          }
          cParam.updateConstraints(width, param.mWidthMode, height, param.mHeightMode);
        } else if (mMode.equals(MODE_CARRY)) {
          float totalMargin = mPreviousMargin + mNextMargin + mPageMargin * 2;
          cParam.updateConstraints((param.mWidth - (mIsVertical ? 0 : totalMargin)) * mXScale,
              param.mWidthMode, (param.mHeight - (mIsVertical ? totalMargin : 0)) * mYScale,
              param.mHeightMode);
        } else {
          cParam.updateConstraints(
              param.mWidth, param.mWidthMode, param.mHeight, param.mHeightMode);
        }
        child.measureNativeNode(context, cParam);
      }
    }
    return new MeasureResult(param.mWidth, param.mHeight);
  }

  @Override
  public void align(AlignParam param, AlignContext context) {
    for (int i = 0; i < this.getChildCount(); i++) {
      ShadowNode node = this.getChildAt(i);
      if (node instanceof NativeLayoutNodeRef) {
        AlignParam align = new AlignParam();
        ((NativeLayoutNodeRef) node).alignNativeNode(context, align);
      }
    }
  }
}
