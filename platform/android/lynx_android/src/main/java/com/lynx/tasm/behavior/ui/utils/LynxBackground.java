// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.utils;

import android.graphics.Color;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.ui.UIShadowProxy;

public class LynxBackground extends LynxDrawableManager<BackgroundDrawable> {
  //  @NonNull protected LynxContext mLynxContext;

  private int mColor;

  public LynxBackground(LynxContext context) {
    super(context);
    this.mColor = 0;
  }

  @Override
  protected BackgroundDrawable createLayerDrawable() {
    return new BackgroundDrawable(mContext, mFontSize);
  }

  public void setBackgroundColor(int color) {
    mColor = color;
    if (color == Color.TRANSPARENT && mLayerDrawable == null) {
      // don't do anything, no need to allocate ReactBackgroundDrawable for transparent background
    } else {
      getOrCreateViewLayer().setColor(color);
    }
  }

  @Nullable
  @Override
  public BackgroundDrawable getDrawable() {
    return super.getDrawable();
  }

  public boolean setBorderRadius(int index, @Nullable ReadableArray ra) {
    if (null == ra || ra.size() <= 0) {
      if (index == 0) {
        for (int i = 0; i < 4; i++) {
          BorderRadius.Corner radius = new BorderRadius.Corner();
          setBorderRadiusCorner(i + 1, radius);
        }
      } else {
        BorderRadius.Corner radius = new BorderRadius.Corner();
        setBorderRadiusCorner(index, radius);
      }
      // indicate the border is none
      return false;
    }
    //@see computed_css_style.cc#borderRadiusToLepus.
    if (index == 0) {
      LLog.DCHECK(ra.size() == 16);
      for (int i = 0; i < 4; i++) {
        setBorderRadiusCorner(i + 1, BorderRadius.Corner.toCorner(ra, i * 4));
      }
    } else {
      LLog.DCHECK(ra.size() == 4);
      setBorderRadiusCorner(index, BorderRadius.Corner.toCorner(ra, 0));
    }
    return true;
  }

  public void setBorderColorForSpacingIndex(int spacingIndex, Integer color) {
    float rgbComponent = color == null ? MeasureUtils.UNDEFINED : (float) (color & 0x00FFFFFF);
    float alphaComponent = color == null ? MeasureUtils.UNDEFINED : (float) (color >>> 24);
    setBorderColor(spacingIndex, rgbComponent, alphaComponent);
  }

  public void setBorderColor(int position, float color, float alpha) {
    getOrCreateViewLayer().setBorderColor(position, color, alpha);
  }

  public void setBorderRadiusCorner(int position, BorderRadius.Corner corner) {
    getOrCreateViewLayer().setBorderRadiusCorner(position, corner);
  }

  public void setBorderStyle(int position, int style) {
    getOrCreateViewLayer().setBorderStyle(position, style);
  }

  public int getBackgroundColor() {
    return mColor;
  }

  public BorderRadius getBorderRadius() {
    if (mLayerDrawable == null) {
      return null;
    }
    return mLayerDrawable.getBorderRadius();
  }

  public void setBoxShadowInsetDrawer(UIShadowProxy.InsetDrawer drawer) {
    getOrCreateViewLayer().setBoxShadowInsetDrawer(drawer);
  }

  public UIShadowProxy.InsetDrawer getBoxShadowInsetDrawer() {
    return mLayerDrawable != null ? mLayerDrawable.getBoxShadowInsetDrawer() : null;
  }
}
