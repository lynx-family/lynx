// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import static android.view.View.LAYER_TYPE_HARDWARE;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.view.View;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;

public class MaskLayerManager extends LayerManager {
  private Paint mMaskPaint = null;

  public MaskLayerManager(LynxContext context, Drawable drawable, float curFontSize) {
    super(context, drawable, curFontSize);
  }

  @Override
  public void draw(Canvas canvas, RectF borderRect, RectF paddingRect, RectF contentRect,
      RectF clipBox, Path outerDrawPath, Path innerDrawPath, boolean hasBorder) {
    int layer = canvas.saveLayer(null, mMaskPaint, Canvas.ALL_SAVE_FLAG);
    super.draw(canvas, borderRect, paddingRect, contentRect, clipBox, outerDrawPath, innerDrawPath,
        hasBorder);
    canvas.restoreToCount(layer);
  }

  @Override
  protected boolean isMask() {
    return true;
  }

  @Override
  public void setLayerImage(ReadableArray bgImage, LynxBaseUI ui) {
    super.setLayerImage(bgImage, ui);
    if (mMaskPaint == null) {
      mMaskPaint = new Paint();
      mMaskPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.DST_IN));
      mMaskPaint.setAntiAlias(true);
      if (ui instanceof LynxUI) {
        View drawView = ((LynxUI<?>) ui).getView();
        if (drawView != null && drawView.getLayerType() != LAYER_TYPE_HARDWARE) {
          drawView.setWillNotDraw(false);
          drawView.setLayerType(LAYER_TYPE_HARDWARE, null);
        }
      }
    }
  }
}
