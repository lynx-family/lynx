// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.utils;

import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.ui.LynxBaseUI;

public abstract class LynxDrawableManager<T extends LayerDrawable<?>> {
  protected final LynxContext mContext;

  @Nullable protected T mLayerDrawable;
  @Nullable private Drawable.Callback mDrawableCallback = null;
  @NonNull protected float mFontSize;

  public LynxDrawableManager(LynxContext context) {
    this.mContext = context;
  }

  public void setDrawableCallback(Drawable.Callback callback) {
    mDrawableCallback = callback;
  }

  protected T getOrCreateViewLayer() {
    if (mLayerDrawable == null) {
      mLayerDrawable = createLayerDrawable();
      mLayerDrawable.setCallback(mDrawableCallback);
    }
    return mLayerDrawable;
  }

  protected abstract T createLayerDrawable();

  @Nullable
  public T getDrawable() {
    return mLayerDrawable;
  }

  public void setBitmapConfig(@Nullable Bitmap.Config config) {
    getOrCreateViewLayer().setBitmapConfig(config);
  }

  public void setLayerImage(@Nullable ReadableArray bgImage, LynxBaseUI ui) {
    getOrCreateViewLayer().setLayerImage(bgImage, ui);
  }

  public void setLayerPosition(ReadableArray backgroundPos) {
    getOrCreateViewLayer().setLayerPosition(backgroundPos);
  }

  public void setLayerSize(ReadableArray bgSize) {
    getOrCreateViewLayer().setLayerSize(bgSize);
  }

  public void setLayerOrigin(ReadableArray bgOrigin) {
    getOrCreateViewLayer().setLayerOrigin(bgOrigin);
  }

  public void setLayerRepeat(ReadableArray bgRepeat) {
    getOrCreateViewLayer().setLayerRepeat(bgRepeat);
  }

  public void setBorderWidth(int position, float width) {
    getOrCreateViewLayer().setBorderWidth(position, width);
  }

  public void setLayerClip(ReadableArray bgClip) {
    getOrCreateViewLayer().setLayerClip(bgClip);
  }

  public void updatePaddingWidths(float top, float right, float bottom, float left) {
    T layer = mLayerDrawable;
    if (layer != null) {
      layer.setPaddingWidth(top, right, bottom, left);
    }
  }

  public void onAttach() {
    if (mLayerDrawable == null)
      return;

    mLayerDrawable.onAttach();
  }

  public void onDetach() {
    if (mLayerDrawable == null)
      return;

    mLayerDrawable.onDetach();
  }

  public float getFontSize() {
    return mFontSize;
  }

  public void setFontSize(float mFontSize) {
    this.mFontSize = mFontSize;
  }

  public void setEnableBitmapGradient(boolean enable) {
    getOrCreateViewLayer().setEnableBitmapGradient(enable);
  }
}
