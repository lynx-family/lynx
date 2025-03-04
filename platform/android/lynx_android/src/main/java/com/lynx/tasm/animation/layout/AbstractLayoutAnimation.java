// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.animation.layout;

import android.graphics.Rect;
import android.view.animation.Animation;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.animation.AnimationInfo;
import com.lynx.tasm.animation.InterpolatorFactory;
import com.lynx.tasm.behavior.ui.LynxUI;

abstract public class AbstractLayoutAnimation {
  public AbstractLayoutAnimation() {
    mInfo = new AnimationInfo();
  }

  boolean isValid() {
    return null != mInfo && mInfo.getDuration() > 0;
  }

  /**
   * Create an animation object for the current animation type, based on the view and final screen
   * coordinates. If the application-supplied configuration does not specify an animation definition
   * for this types, or if the animation definition is invalid, returns null.
   */
  abstract @Nullable Animation createAnimationImpl(LynxUI ui, int x, int y, int width, int height,
      int paddingLeft, int paddingTop, int paddingRight, int paddingBottom, int marginLeft,
      int marginTop, int marginRight, int marginBottom, int borderLeftWidth, int borderTopWidth,
      int borderRightWidth, int borderBottomWidth, final Rect bound, float originAlpha);

  protected AnimationInfo mInfo;

  protected void reset() {
    mInfo.setDuration(0);
  }

  /**
   * Create an animation object to be used to animate the view, based on the animation config
   * supplied at initialization time and the new view position and size.
   */
  protected final @Nullable Animation createAnimation(LynxUI ui, int x, int y, int width,
      int height, int paddingLeft, int paddingTop, int paddingRight, int paddingBottom,
      int marginLeft, int marginTop, int marginRight, int marginBottom, int borderLeftWidth,
      int borderTopWidth, int borderRightWidth, int borderBottomWidth, final Rect bound,
      float originAlpha) {
    if (!isValid()) {
      return null;
    }
    Animation animation = createAnimationImpl(ui, x, y, width, height, paddingLeft, paddingTop,
        paddingRight, paddingBottom, marginLeft, marginTop, marginRight, marginBottom,
        borderLeftWidth, borderTopWidth, borderRightWidth, borderBottomWidth, bound, originAlpha);
    if (animation != null) {
      animation.setDuration(mInfo.getDuration());
      animation.setStartOffset(mInfo.getDelay());
      animation.setInterpolator(InterpolatorFactory.getInterpolator(mInfo));
    }
    return animation;
  }

  public void setInterpolator(@Nullable ReadableArray interpolator) {
    mInfo.setTimingFunction(interpolator, 0);
  }

  public void setInterpolator(
      int timingType, float x1, float y1, float x2, float y2, int stepsType) {
    mInfo.setTimingFunction(timingType, x1, y1, x2, y2, stepsType);
  }

  public void setAnimatedProperty(@Nullable int animatedProperty) {
    mInfo.setProperty(animatedProperty);
  }

  public void setDelay(@Nullable long delay) {
    mInfo.setDelay(delay);
  }

  public void setDuration(@Nullable long duration) {
    mInfo.setDuration(duration);
  }
}
