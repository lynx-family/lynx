// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.swiper;

import static com.lynx.tasm.behavior.ui.swiper.XSwiperUI.SELECTED_COLOR;
import static com.lynx.tasm.behavior.ui.swiper.XSwiperUI.UNSELECTED_COLOR;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.GradientDrawable;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import com.lynx.tasm.behavior.ui.IDrawChildHook;
import com.lynx.tasm.utils.PixelUtils;

class SwiperView extends FrameLayout implements IDrawChildHook.IDrawChildHookBinding {
  public static final String TAG = "LynxSwiperUI#SwiperView";
  public static final boolean DEBUG = false;
  private final ViewPager mViewPager;
  private final LinearLayout mIndicators;
  private final int mIndicatorSize;
  private int mSelectedColor = SELECTED_COLOR;
  private int mUnselectedColor = UNSELECTED_COLOR;
  private int mSelectedIndex = 0;
  public static final int ORIENTATION_HORIZONTAL = 0;
  public static final int ORIENTATION_VERTICAL = 1;
  private int mOrientation = ORIENTATION_HORIZONTAL;
  private IDrawChildHook mDrawChildHook;

  public SwiperView(@NonNull Context context) {
    this(context, null);
  }

  public SwiperView(@NonNull Context context, @Nullable AttributeSet attrs) {
    super(context, attrs);
    mIndicatorSize = (int) PixelUtils.dipToPx(7);
    mViewPager = new ViewPager(context);
    addView(mViewPager,
        new LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    mIndicators = new LinearLayout(context);
    updateIndicators(ORIENTATION_HORIZONTAL);
  }

  private void updateIndicators(int orientation) {
    LayoutParams params;
    if (orientation == ORIENTATION_VERTICAL) {
      mIndicators.setGravity(Gravity.CENTER_VERTICAL);
      mIndicators.setOrientation(LinearLayout.VERTICAL);
      params = new LayoutParams(
          ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT);
      params.rightMargin = (int) PixelUtils.dipToPx(10);
      params.gravity = Gravity.CENTER_VERTICAL | Gravity.END;
    } else {
      mIndicators.setGravity(Gravity.CENTER);
      mIndicators.setOrientation(LinearLayout.HORIZONTAL);
      params = new LayoutParams(
          ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
      params.bottomMargin = (int) PixelUtils.dipToPx(10);
      params.gravity = Gravity.CENTER_HORIZONTAL | Gravity.BOTTOM;
    }
    addView(mIndicators, params);
    int indicatorCount = mIndicators.getChildCount();
    mIndicators.removeAllViews();
    for (int i = 0; i < indicatorCount; ++i) {
      addIndicator();
    }
    setSelectedInternal();
  }

  public ViewPager getViewPager() {
    return mViewPager;
  }

  public void addIndicator() {
    View view = new View(getContext());
    view.setClickable(false);
    LinearLayout.LayoutParams params =
        new LinearLayout.LayoutParams(mIndicatorSize, mIndicatorSize);
    if (mOrientation == ORIENTATION_VERTICAL) {
      params.topMargin = params.bottomMargin = mIndicatorSize / 2;
    } else {
      params.leftMargin = params.rightMargin = mIndicatorSize / 2;
    }
    mIndicators.addView(view, params);
    if (mIndicators.getChildCount() == mSelectedColor) {
      view.setBackground(createIndicatorDrawable(mSelectedColor));
    } else {
      view.setBackground(createIndicatorDrawable(mUnselectedColor));
    }
  }

  public void removeIndicator() {
    if (mIndicators.getChildCount() > 0) {
      mIndicators.removeViewAt(0);
      setSelected(mSelectedIndex);
    }
  }

  private GradientDrawable createIndicatorDrawable(int color) {
    GradientDrawable gradientDrawable = new GradientDrawable();
    gradientDrawable.setShape(GradientDrawable.OVAL);
    gradientDrawable.setColor(color);
    return gradientDrawable;
  }

  public void setSelectedColor(int color) {
    mSelectedColor = color;
    for (int i = mIndicators.getChildCount() - 1; i > -1; i--) {
      if (i == mSelectedIndex) {
        View child = mIndicators.getChildAt(i);
        child.setBackground(createIndicatorDrawable(mSelectedColor));
        return;
      }
    }
  }

  public void setUnSelectedColor(int color) {
    mUnselectedColor = color;
    for (int i = mIndicators.getChildCount() - 1; i > -1; i--) {
      if (i == mSelectedIndex) {
        continue;
      }
      View child = mIndicators.getChildAt(i);
      child.setBackground(createIndicatorDrawable(mUnselectedColor));
    }
  }

  public void setSelected(int index) {
    mSelectedIndex = index;
    setSelectedInternal();
  }

  private void setSelectedInternal() {
    for (int i = mIndicators.getChildCount() - 1; i > -1; i--) {
      View child = mIndicators.getChildAt(i);
      if (i == mSelectedIndex) {
        child.setBackground(createIndicatorDrawable(mSelectedColor));
      } else {
        child.setBackground(createIndicatorDrawable(mUnselectedColor));
      }
    }
  }

  public void enableIndicator(boolean b) {
    mIndicators.setVisibility(b ? VISIBLE : GONE);
  }

  public void setIsRtl(boolean isRtl) {
    if (isRtl) {
      ViewCompat.setLayoutDirection(mIndicators, ViewCompat.LAYOUT_DIRECTION_RTL);
    } else {
      ViewCompat.setLayoutDirection(mIndicators, ViewCompat.LAYOUT_DIRECTION_LTR);
    }
    mViewPager.setIsRTL(isRtl);
  }

  public void setOrientation(int orientation) {
    mOrientation = orientation;
    if (mIndicators != null) {
      removeView(mIndicators);
      updateIndicators(orientation);
    }
    getViewPager().setOrientation(orientation);
  }

  @Override
  public void bindDrawChildHook(IDrawChildHook hook) {
    mDrawChildHook = hook;
  }

  @Override
  protected void dispatchDraw(Canvas canvas) {
    // Clip radius before dispatchDraw
    mDrawChildHook.beforeDispatchDraw(canvas);
    super.dispatchDraw(canvas);
  }
}
