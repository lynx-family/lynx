// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.animation.layout;

import android.graphics.Rect;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.ui.LynxUI;

public class LayoutUpdateAnimation extends AbstractLayoutAnimation {
  public LayoutUpdateAnimation() {
    super();
  }

  @Override
  @Nullable
  Animation createAnimationImpl(LynxUI ui, int x, int y, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int marginLeft, int marginTop,
      int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, final Rect bound, float originAlpha) {
    View view = ui.getView();
    boolean animateLocation = ui.getOriginLeft() != x || ui.getOriginTop() != y;
    boolean animateSize = ui.getWidth() != width || ui.getHeight() != height;
    if (!animateLocation && !animateSize) {
      return null;
    } else if (animateLocation && !animateSize) {
      return new TranslateAnimation(ui.getOriginLeft() - x, 0, ui.getOriginTop() - y, 0);
    } else {
      return new PositionAndSizeAnimation(ui, x, y, width, height, paddingLeft, paddingTop,
          paddingRight, paddingBottom, marginLeft, marginTop, marginRight, marginBottom,
          borderLeftWidth, borderTopWidth, borderRightWidth, borderBottomWidth, bound);
    }
  }
}
