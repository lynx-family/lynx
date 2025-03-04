// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.utils;

import android.graphics.PointF;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.os.Build;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxUI;
import java.lang.ref.WeakReference;
import java.util.List;

/** Class that manages the background for views and borders. */
public class BackgroundManager extends LynxBackground {
  private WeakReference<LynxUI> mUI;
  private TransformProps mTransformProps;
  private PointF mPostTranslate = null;
  private float mTranslateZ;
  private boolean mEnableTransformOrder = true;

  public BackgroundManager(LynxUI ui, LynxContext lynxContext) {
    super(lynxContext);
    mUI = new WeakReference<>(ui);
  }

  @Override
  protected BackgroundDrawable createLayerDrawable() {
    LynxUI ui = mUI.get();
    if (ui == null) {
      return null;
    }
    BackgroundDrawable backgroundLayerDrawable = super.createLayerDrawable();
    if (ui.getView() != null) {
      Drawable viewBackgroundDrawable = ui.getView().getBackground();
      ViewHelper.setBackground(ui.getView(),
          null); // required so that drawable callback is cleared before we add the
      // drawable back as a part of LayerDrawable
      if (viewBackgroundDrawable == null) {
        ViewHelper.setBackground(ui.getView(), backgroundLayerDrawable);
      } else {
        LayerDrawable layerDrawable =
            new LayerDrawable(new Drawable[] {backgroundLayerDrawable, viewBackgroundDrawable});
        ViewHelper.setBackground(ui.getView(), layerDrawable);
      }
    }
    return backgroundLayerDrawable;
  }

  public static float convertAngle(String angle) {
    if (angle.endsWith("deg")) {
      return Float.valueOf(angle.substring(0, angle.length() - 3));
    } else if (angle.endsWith("rad")) {
      return Float.valueOf(angle.substring(0, angle.length() - 3)) * 180 / (float) Math.PI;
    } else if (angle.endsWith("turn")) {
      return Float.valueOf(angle.substring(0, angle.length() - 4)) * 360;
    }
    return 0;
  }

  private void resetTransform() {
    LynxUI ui = mUI.get();
    if (ui == null || ui.getView() == null) {
      return;
    }
    ui.getView().setTranslationX(0);
    ui.getView().setTranslationY(0);
    ui.getView().setRotation(0);
    ui.getView().setRotationX(0);
    ui.getView().setRotationY(0);
    ui.getView().setScaleX(1);
    ui.getView().setScaleY(1);
    mTransformProps = null;
    updateViewTranslation();
  }

  public void setPostTranlate(PointF tranlate) {
    mPostTranslate = tranlate;
    updateViewTranslation();
  }

  public void setTransformOrigin(TransformOrigin transformOrigin) {
    LynxUI ui = mUI.get();
    if (ui == null || ui.getView() == null) {
      return;
    }
    if (transformOrigin == null) {
      return;
    }
    TransformProps transformOriginProps = TransformProps.processTransformOrigin(
        transformOrigin, ui.getLatestWidth(), ui.getLatestHeight());
    ui.getView().setPivotX(transformOriginProps.getTransformOriginX());
    ui.getView().setPivotY(transformOriginProps.getTransformOriginY());
    ui.getView().invalidate();
  }

  public void setTransform(@Nullable List<TransformRaw> styleProperty) {
    LynxUI ui = mUI.get();
    if (ui == null) {
      return;
    }
    // Reset transform firstly
    resetTransform();
    if (styleProperty == null) {
      return;
    }

    if (mEnableTransformOrder) {
      mTransformProps = TransformProps.processTransformInOrder(styleProperty,
          mContext.getUIBody().getFontSize(), mFontSize, mContext.getUIBody().getLatestWidth(),
          mContext.getUIBody().getLatestHeight(), ui.getLatestWidth(), ui.getLatestHeight());
    } else {
      mTransformProps = TransformProps.processTransform(styleProperty,
          mContext.getUIBody().getFontSize(), mFontSize, mContext.getUIBody().getLatestWidth(),
          mContext.getUIBody().getLatestHeight(), ui.getLatestWidth(), ui.getLatestHeight());
    }

    updateViewTranslation();

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      ui.getView().setOutlineProvider(null); // disable shadow
      ViewCompat.setTranslationZ(ui.getView(), mTransformProps.getTranslationZ());
    } else {
      // prior to API 21, use zIndex instead
      // TODO(linxiaosong): fix the translateZ
      // mTranslateZ = mMatrixDecompositionContext.getTranslationZ();
      // ui.setZIndex(Math.round(mTranslateZ));
    }
    ui.getView().setRotation(mTransformProps.getRotation());
    ui.getView().setRotationX(mTransformProps.getRotationX());
    ui.getView().setRotationY(mTransformProps.getRotationY());
    ui.getView().setScaleX(mTransformProps.getScaleX());
    ui.getView().setScaleY(mTransformProps.getScaleY());
    ui.setSkewX(mTransformProps.getSkewX());
    ui.setSkewY(mTransformProps.getSkewY());
    ui.getView().invalidate();
  }

  private void updateViewTranslation() {
    LynxUI ui = mUI.get();
    if (ui == null) {
      return;
    }
    float x = 0, y = 0;
    if (mPostTranslate != null) {
      x += mPostTranslate.x;
      y += mPostTranslate.y;
    }
    if (mTransformProps != null) {
      x += mTransformProps.getTranslationX();
      y += mTransformProps.getTranslationY();
    }
    if (ui.getView() != null) {
      ui.getView().setTranslationX(x);
      ui.getView().setTranslationY(y);
    }
  }

  public float getTranslateZ() {
    return mTranslateZ;
  }

  public TransformProps getTransformProps() {
    return mTransformProps;
  }

  public void setTransformOrder(boolean transformOrder) {
    mEnableTransformOrder = transformOrder;
  }
}
