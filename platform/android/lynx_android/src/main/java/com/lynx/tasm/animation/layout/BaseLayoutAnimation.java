// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.animation.layout;

import android.graphics.Rect;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.ScaleAnimation;
import com.lynx.tasm.animation.AnimationConstant;
import com.lynx.tasm.behavior.ui.LynxUI;

abstract public class BaseLayoutAnimation extends AbstractLayoutAnimation {
  abstract boolean isReverse();

  @Override
  Animation createAnimationImpl(LynxUI ui, int x, int y, int width, int height, int paddingLeft,
      int paddingTop, int paddingRight, int paddingBottom, int marginLeft, int marginTop,
      int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, final Rect bound, float originAlpha) {
    View view = ui.getView();
    switch (mInfo.getProperty()) {
      case AnimationConstant.PROP_SCALE_X_Y: {
        float fromValue = isReverse() ? 1.0f : 0.0f;
        float toValue = isReverse() ? 0.0f : 1.0f;
        return new ScaleAnimation(fromValue, toValue, fromValue, toValue,
            Animation.RELATIVE_TO_SELF, .5f, Animation.RELATIVE_TO_SELF, .5f);
      }
      case AnimationConstant.PROP_SCALE_X: {
        float fromValue = isReverse() ? 1.0f : 0.0f;
        float toValue = isReverse() ? 0.0f : 1.0f;
        return new ScaleAnimation(fromValue, toValue, 1f, 1f, Animation.RELATIVE_TO_SELF, .5f,
            Animation.RELATIVE_TO_SELF, 0f);
      }
      case AnimationConstant.PROP_SCALE_Y: {
        float fromValue = isReverse() ? 1.0f : 0.0f;
        float toValue = isReverse() ? 0.0f : 1.0f;
        return new ScaleAnimation(1f, 1f, fromValue, toValue, Animation.RELATIVE_TO_SELF, 0f,
            Animation.RELATIVE_TO_SELF, .5f);
      }
      default: {
        float fromValue = isReverse() ? originAlpha : 0.0f;
        float toValue = isReverse() ? 0.0f : originAlpha;
        return new OpacityAnimation(view, fromValue, toValue);
      }
    }
  }
}
