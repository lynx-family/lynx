// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static com.lynx.tasm.behavior.StyleConstants.FILTER_TYPE_BLUR;
import static com.lynx.tasm.behavior.StyleConstants.FILTER_TYPE_BRIGHTNESS;
import static com.lynx.tasm.behavior.StyleConstants.FILTER_TYPE_CONTRAST;
import static com.lynx.tasm.behavior.StyleConstants.FILTER_TYPE_GRAYSCALE;
import static com.lynx.tasm.behavior.StyleConstants.FILTER_TYPE_HUE_ROTATE;
import static com.lynx.tasm.behavior.StyleConstants.FILTER_TYPE_NONE;
import static com.lynx.tasm.behavior.StyleConstants.FILTER_TYPE_SATURATE;

import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.graphics.Rect;
import android.view.View;
import android.view.ViewGroup;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.utils.BlurUtils;
import com.lynx.tasm.utils.UnitUtils;

public class ContainerRenderer extends ViewGroup implements IRendererHost {
  @Override
  public Renderer createRenderer(PlatformRendererContext platformRendererContext, int sign) {
    return new Renderer(platformRendererContext, sign);
  }

  private Renderer mRenderer;
  private int mRendererHostFilterType = FILTER_TYPE_NONE;

  @Override
  public void setRenderer(Renderer renderer) {
    mRenderer = renderer;
  }

  @Override
  public Renderer getRenderer() {
    return mRenderer;
  }

  @Override
  public View getView() {
    return this;
  }

  public ContainerRenderer(LynxContext context) {
    super(context);
    setWillNotDraw(false);
    setClipChildren(false);
  }

  @Override
  public void applyRendererHostFilter(int type, float amount) {
    if (mRendererHostFilterType == FILTER_TYPE_BLUR && type != FILTER_TYPE_BLUR) {
      BlurUtils.removeEffect(this);
    }

    switch (type) {
      case FILTER_TYPE_NONE:
        clearRendererHostFilter();
        break;
      case FILTER_TYPE_GRAYSCALE:
        amount = UnitUtils.clamp(1.0f - amount, 0.0f, 1.0f);
        ColorMatrix grayscaleMatrix = new ColorMatrix();
        grayscaleMatrix.setSaturation(amount);
        applyRendererColorFilter(grayscaleMatrix);
        break;
      case FILTER_TYPE_BLUR:
        setLayerType(View.LAYER_TYPE_NONE, null);
        amount = Math.max(0.0f, amount);
        if (amount == 0.0f) {
          BlurUtils.removeEffect(this);
        } else {
          BlurUtils.createEffect(this, amount);
        }
        break;
      case FILTER_TYPE_BRIGHTNESS:
        amount = UnitUtils.clamp(amount, 0.0f, 2.0f);
        ColorMatrix brightnessMatrix = new ColorMatrix();
        brightnessMatrix.setScale(amount, amount, amount, 1.0f);
        applyRendererColorFilter(brightnessMatrix);
        break;
      case FILTER_TYPE_CONTRAST:
        amount = UnitUtils.clamp(amount, 0.0f, 3.0f);
        float offset = 128.0f * (1.0f - amount);
        ColorMatrix contrastMatrix = new ColorMatrix(new float[] {amount, 0, 0, 0, offset, 0,
            amount, 0, 0, offset, 0, 0, amount, 0, offset, 0, 0, 0, 1, 0});
        applyRendererColorFilter(contrastMatrix);
        break;
      case FILTER_TYPE_SATURATE:
        amount = amount < 0.0f ? 1.0f : Math.min(3.0f, amount);
        ColorMatrix saturateMatrix = new ColorMatrix();
        saturateMatrix.setSaturation(amount);
        applyRendererColorFilter(saturateMatrix);
        break;
      case FILTER_TYPE_HUE_ROTATE:
      default:
        clearRendererHostFilter();
        break;
    }
    mRendererHostFilterType = type;
  }

  private void applyRendererColorFilter(ColorMatrix colorMatrix) {
    Paint filterPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    filterPaint.setColorFilter(new ColorMatrixColorFilter(colorMatrix));
    setLayerType(View.LAYER_TYPE_HARDWARE, filterPaint);
  }

  private void clearRendererHostFilter() {
    setLayerType(View.LAYER_TYPE_NONE, null);
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    Rect frame = mRenderer.getLynxFrame();
    setMeasuredDimension(frame.width(), frame.height());
    mRenderer.onMeasure(widthMeasureSpec, heightMeasureSpec);
  }

  @Override
  protected void onLayout(boolean changed, int l, int t, int r, int b) {
    mRenderer.onLayout(changed, l, t, r, b);
  }

  @Override
  protected void onDraw(Canvas canvas) {
    mRenderer.onDraw(canvas);
  }

  @Override
  protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
    mRenderer.beforeDrawChild(canvas, child);
    boolean ret = super.drawChild(canvas, child, drawingTime);
    mRenderer.afterDrawChild(canvas, child);
    return ret;
  }

  @Override
  protected void dispatchDraw(Canvas canvas) {
    super.dispatchDraw(canvas);
    mRenderer.afterDispatchDraw(canvas);
  }
}
