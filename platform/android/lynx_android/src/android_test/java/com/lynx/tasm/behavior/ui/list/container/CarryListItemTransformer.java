// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list.container;

import android.view.View;
import androidx.annotation.NonNull;
import androidx.core.math.MathUtils;
import java.util.WeakHashMap;

/**
 * Scales and translates list items as they move away from the viewport's leading edge.
 *
 * <p>This mirrors XSwiper's carry-mode transformation while leaving scrolling and paging to the
 * list container.
 */
public class CarryListItemTransformer implements ListItemTransformer {
  private float mNormTranslationFactor = 0.f;
  private float mMinScaleX = 0.8f;
  private float mMaxScaleX = 1.0f;
  private float mMinScaleY = 0.8f;
  private float mMaxScaleY = 1.0f;
  // Preserve translation owned by sticky positioning or another source while Carry translation is
  // disabled.
  private final WeakHashMap<View, Boolean> mTranslatedItems = new WeakHashMap<>();

  @Override
  public void transformItem(@NonNull ListContainerView listContainerView, @NonNull View itemView,
      boolean isVertical, boolean isRTL, int mainAxisOffset) {
    int mainAxisSize = isVertical ? itemView.getHeight() : itemView.getWidth();
    if (mainAxisSize <= 0) {
      mainAxisSize = isVertical ? listContainerView.getHeight() : listContainerView.getWidth();
    }
    if (mainAxisSize <= 0 || mMaxScaleX < mMinScaleX || mMaxScaleY < mMinScaleY) {
      resetItem(itemView);
      return;
    }

    float distance = Math.abs((float) mainAxisOffset);
    float scaleX = mMaxScaleX - distance * (mMaxScaleX - mMinScaleX) / mainAxisSize;
    float scaleY = mMaxScaleY - distance * (mMaxScaleY - mMinScaleY) / mainAxisSize;
    scaleX = MathUtils.clamp(scaleX, mMinScaleX, mMaxScaleX);
    scaleY = MathUtils.clamp(scaleY, mMinScaleY, mMaxScaleY);
    itemView.setScaleX(scaleX);
    itemView.setScaleY(scaleY);

    if (mNormTranslationFactor > 0.f) {
      mTranslatedItems.put(itemView, Boolean.TRUE);
      float translation = calculateMainAxisTranslation(mainAxisOffset, mainAxisSize, isVertical);
      if (isVertical) {
        itemView.setTranslationX(0.f);
        itemView.setTranslationY(translation);
      } else {
        itemView.setTranslationX(isRTL ? -translation : translation);
        itemView.setTranslationY(0.f);
      }
    } else if (mTranslatedItems.remove(itemView) != null) {
      itemView.setTranslationX(0.f);
      itemView.setTranslationY(0.f);
    }
  }

  private float calculateMainAxisTranslation(
      int mainAxisOffset, int mainAxisSize, boolean isVertical) {
    float maxScale = isVertical ? mMaxScaleY : mMaxScaleX;
    float minScale = isVertical ? mMinScaleY : mMinScaleX;
    float interval = mNormTranslationFactor * mainAxisSize * (2.0f - maxScale - minScale) / 2.f;
    float distance = Math.min(Math.abs((float) mainAxisOffset), mainAxisSize);
    float halfSize = mainAxisSize / 2.f;
    float normFactor = 1.0f - Math.abs(distance - halfSize) / halfSize;
    if (mainAxisOffset > 0) {
      return distance >= halfSize ? -interval + 0.5f * normFactor * interval
                                  : -0.5f * normFactor * interval;
    }
    return distance <= halfSize ? 0.5f * normFactor * interval
                                : interval - 0.5f * normFactor * interval;
  }

  @Override
  public void resetItem(@NonNull View itemView) {
    itemView.setScaleX(1.f);
    itemView.setScaleY(1.f);
    if (mTranslatedItems.remove(itemView) != null) {
      itemView.setTranslationX(0.f);
      itemView.setTranslationY(0.f);
    }
  }

  public void setNormTranslationFactor(float normTranslationFactor) {
    mNormTranslationFactor = normTranslationFactor;
  }

  public void setMinScaleX(float minScaleX) {
    mMinScaleX = minScaleX;
  }

  public void setMaxScaleX(float maxScaleX) {
    mMaxScaleX = maxScaleX;
  }

  public void setMinScaleY(float minScaleY) {
    mMinScaleY = minScaleY;
  }

  public void setMaxScaleY(float maxScaleY) {
    mMaxScaleY = maxScaleY;
  }
}
