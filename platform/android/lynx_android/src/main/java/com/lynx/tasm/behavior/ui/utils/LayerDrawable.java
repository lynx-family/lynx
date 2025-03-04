// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.utils;

import android.graphics.Bitmap;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.background.LayerManager;
import com.lynx.tasm.utils.FloatUtils;
import java.util.Arrays;

public abstract class LayerDrawable<T extends LayerManager> extends Drawable {
  /*
   * Value at Spacing.ALL index used for rounded borders, whole array used by
   * rectangular borders
   */
  protected @NonNull final Spacing mBorderWidth;
  protected @NonNull final float[] mPaddingWidth;

  protected final Rect mPaddingBox = new Rect();
  protected final Rect mContentBox = new Rect();

  protected T mLayerManager = null;

  protected final LynxContext mContext;
  private int mLayoutDirection;
  protected float mCurFontSize;

  public LayerDrawable(LynxContext lynxContext, float curFontSize) {
    mContext = lynxContext;
    mCurFontSize = curFontSize;
    mLayerManager = createLayerManager();
    // init BorderWidth and PaddingWidth
    mPaddingWidth = new float[4];
    Arrays.fill(mPaddingWidth, 0);
    if (mContext.getCssAlignWithLegacyW3c()) {
      mBorderWidth = new Spacing(BorderWidth.MEDIUM);
    } else {
      mBorderWidth = new Spacing();
    }
  }

  protected abstract T createLayerManager();

  @Override
  protected void onBoundsChange(Rect bounds) {
    super.onBoundsChange(bounds);
    mLayerManager.configureBounds(bounds);
    updateContentBox();
  }

  void setBitmapConfig(@Nullable Bitmap.Config config) {
    if (mLayerManager == null) {
      return;
    }
    mLayerManager.setBitmapConfig(config);
  }

  public void setLayerImage(ReadableArray backgroundImage, LynxBaseUI ui) {
    mLayerManager.setLayerImage(backgroundImage, ui);
  }

  public void setLayerPosition(ReadableArray backgroundPos) {
    mLayerManager.setLayerPosition(backgroundPos);
  }

  public void setLayerSize(ReadableArray bgSize) {
    mLayerManager.setLayerSize(bgSize);
  }

  public void setLayerOrigin(ReadableArray bgOrigin) {
    mLayerManager.setLayerOrigin(bgOrigin);
  }

  public void setLayerRepeat(ReadableArray bgRepeat) {
    mLayerManager.setLayerRepeat(bgRepeat);
  }

  public void setLayerClip(ReadableArray bgClip) {
    mLayerManager.setLayerClip(bgClip);
  }

  public boolean setBorderWidth(int position, float width) {
    return mBorderWidth.set(position, width);
  }

  public boolean setPaddingWidth(float top, float right, float bottom, float left) {
    boolean changed = false;
    if (!FloatUtils.floatsEqual(mPaddingWidth[StyleConstants.BORDER_SIDE_TOP], top)) {
      mPaddingWidth[StyleConstants.BORDER_SIDE_TOP] = top;
      changed = true;
    }
    if (!FloatUtils.floatsEqual(mPaddingWidth[StyleConstants.BORDER_SIDE_RIGHT], right)) {
      mPaddingWidth[StyleConstants.BORDER_SIDE_RIGHT] = right;
      changed = true;
    }
    if (!FloatUtils.floatsEqual(mPaddingWidth[StyleConstants.BORDER_SIDE_BOTTOM], bottom)) {
      mPaddingWidth[StyleConstants.BORDER_SIDE_BOTTOM] = bottom;
      changed = true;
    }
    if (!FloatUtils.floatsEqual(mPaddingWidth[StyleConstants.BORDER_SIDE_LEFT], left)) {
      mPaddingWidth[StyleConstants.BORDER_SIDE_LEFT] = left;
      changed = true;
    }
    return changed;
  }

  protected void updateContentBox() {
    // Update padding box
    final Rect bounds = getBounds();
    mPaddingBox.set(bounds.left, bounds.top, bounds.right, bounds.bottom);
    RectF borderInsets = getDirectionAwareBorderInsets();
    mPaddingBox.left += borderInsets.left;
    mPaddingBox.top += borderInsets.top;
    mPaddingBox.right -= borderInsets.right;
    mPaddingBox.bottom -= borderInsets.bottom;

    // Update content box
    mContentBox.set(mPaddingBox.left, mPaddingBox.top, mPaddingBox.right, mPaddingBox.bottom);
    mContentBox.top += mPaddingWidth[StyleConstants.BORDER_SIDE_TOP];
    mContentBox.right -= mPaddingWidth[StyleConstants.BORDER_SIDE_RIGHT];
    mContentBox.bottom -= mPaddingWidth[StyleConstants.BORDER_SIDE_BOTTOM];
    mContentBox.left += mPaddingWidth[StyleConstants.BORDER_SIDE_LEFT];
  }

  public int getResolvedLayoutDirection() {
    return mLayoutDirection;
  }

  /** Similar to Drawable.setLayoutDirection, but available in APIs < 23. */
  public boolean setResolvedLayoutDirection(int layoutDirection) {
    if (mLayoutDirection != layoutDirection) {
      mLayoutDirection = layoutDirection;
      return onResolvedLayoutDirectionChanged(layoutDirection);
    }
    return false;
  }

  /** Similar to Drawable.onLayoutDirectionChanged, but available in APIs < 23. */
  public boolean onResolvedLayoutDirectionChanged(int layoutDirection) {
    return false;
  }

  public float getBorderWidthOrDefaultTo(final float defaultValue, final int spacingType) {
    if (mBorderWidth == null) {
      return defaultValue;
    }

    final float width = mBorderWidth.getRaw(spacingType);

    if (MeasureUtils.isUndefined(width)) {
      return defaultValue;
    }

    return width;
  }

  public RectF getDirectionAwareBorderInsets() {
    float borderTopWidth = mBorderWidth.get(Spacing.TOP);
    float borderBottomWidth = mBorderWidth.get(Spacing.BOTTOM);
    float borderLeftWidth = mBorderWidth.get(Spacing.LEFT);
    float borderRightWidth = mBorderWidth.get(Spacing.RIGHT);

    final Rect bounds = getBounds();
    if (borderLeftWidth + borderRightWidth > bounds.width() && bounds.width() >= 1) {
      final float tmp = bounds.width() / (borderLeftWidth + borderRightWidth);
      borderLeftWidth *= tmp;
      borderRightWidth *= tmp;
    }
    if (borderTopWidth + borderBottomWidth > bounds.height() && bounds.height() >= 1) {
      final float tmp = bounds.height() / (borderTopWidth + borderBottomWidth);
      borderTopWidth *= tmp;
      borderBottomWidth *= tmp;
    }

    return new RectF(borderLeftWidth, borderTopWidth, borderRightWidth, borderBottomWidth);
  }

  public void onAttach() {
    mLayerManager.onAttach();
  }

  public void onDetach() {
    mLayerManager.onDetach();
  }

  public void setEnableBitmapGradient(boolean enable) {
    mLayerManager.setEnableBitmapGradient(enable);
    invalidateSelf();
  }
}
