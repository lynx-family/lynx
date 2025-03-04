// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.swiper;

import android.view.View;
import androidx.core.math.MathUtils;

public class ModeCarryTransformer implements ViewPager.PageTransformer {
  private float mNormTranslationFactor = 0.f;
  private float mMinScaleX = 0.8f;
  private float mMaxScaleX = 1.0f;
  private float mMinScaleY = 0.8f;
  private float mMaxScaleY = 1.0f;

  @Override
  public void transformPage(ViewPager viewPager, View page, boolean isVertical, int offset) {
    if (viewPager != null && page != null && mMaxScaleX >= mMinScaleX && mMaxScaleY >= mMinScaleY
        && viewPager.getChildExpectSize() > 0) {
      float distance = Math.abs(offset);
      float scaleX = 1;
      float scaleY = 1;
      int expectSize = viewPager.getChildExpectSize();
      scaleX = mMaxScaleX - distance * (mMaxScaleX - mMinScaleX) / expectSize;
      scaleY = mMaxScaleY - distance * (mMaxScaleY - mMinScaleY) / expectSize;
      scaleX = MathUtils.clamp(scaleX, mMinScaleX, mMaxScaleX);
      scaleY = MathUtils.clamp(scaleY, mMinScaleY, mMaxScaleY);

      page.setScaleX(scaleX);
      page.setScaleY(scaleY);

      if (mNormTranslationFactor > 0.f) {
        float interval = 0.f;
        float translation = 0.f;
        if (isVertical) {
          interval = mNormTranslationFactor * expectSize * (2.0f - mMaxScaleY - mMinScaleY) / 2.f;
        } else {
          interval = mNormTranslationFactor * expectSize * (2.0f - mMaxScaleX - mMinScaleX) / 2.f;
        }
        distance = Math.min(distance, expectSize);
        float normFactor = 1.0f - Math.abs(distance - expectSize / 2.f) / (expectSize / 2.f);
        if (offset > 0) {
          // move closer to the center of screen
          if (distance >= expectSize / 2.f) {
            translation = -interval + 0.5f * normFactor * interval;
          } else {
            translation = -0.5f * normFactor * interval;
          }
        } else {
          // move away to the center of screen
          if (distance <= expectSize / 2.f) {
            translation = 0.5f * normFactor * interval;
          } else {
            translation = interval - 0.5f * normFactor * interval;
          }
        }
        if (isVertical) {
          page.setTranslationY(translation);
        } else {
          page.setTranslationX(translation);
        }
      }
    }
  }

  @Override
  public void reset(View page) {
    if (page != null) {
      page.setScaleX(1);
      page.setScaleY(1);
      page.setTranslationX(0);
      page.setTranslationY(0);
    }
  }

  public void setNormTranslationFactor(float normTranslationFactor) {
    this.mNormTranslationFactor = normTranslationFactor;
  }

  public void setMinScaleX(float minScaleX) {
    this.mMinScaleX = minScaleX;
  }

  public void setMaxScaleX(float maxScaleX) {
    this.mMaxScaleX = maxScaleX;
  }

  public void setMinScaleY(float minScaleY) {
    this.mMinScaleY = minScaleY;
  }

  public void setMaxScaleY(float maxScaleY) {
    this.mMaxScaleY = maxScaleY;
  }
}
