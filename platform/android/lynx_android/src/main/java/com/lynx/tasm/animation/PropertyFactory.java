// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.animation;

import static com.lynx.tasm.animation.AnimationConstant.PROP_BACKGROUND_COLOR;
import static com.lynx.tasm.animation.AnimationConstant.PROP_BOTTOM;
import static com.lynx.tasm.animation.AnimationConstant.PROP_HEIGHT;
import static com.lynx.tasm.animation.AnimationConstant.PROP_LEFT;
import static com.lynx.tasm.animation.AnimationConstant.PROP_OPACITY;
import static com.lynx.tasm.animation.AnimationConstant.PROP_RIGHT;
import static com.lynx.tasm.animation.AnimationConstant.PROP_SCALE_X;
import static com.lynx.tasm.animation.AnimationConstant.PROP_SCALE_X_Y;
import static com.lynx.tasm.animation.AnimationConstant.PROP_SCALE_Y;
import static com.lynx.tasm.animation.AnimationConstant.PROP_STR_BOTTOM;
import static com.lynx.tasm.animation.AnimationConstant.PROP_STR_HEIGHT;
import static com.lynx.tasm.animation.AnimationConstant.PROP_STR_LEFT;
import static com.lynx.tasm.animation.AnimationConstant.PROP_STR_OPACITY;
import static com.lynx.tasm.animation.AnimationConstant.PROP_STR_RIGHT;
import static com.lynx.tasm.animation.AnimationConstant.PROP_STR_SCALE_X;
import static com.lynx.tasm.animation.AnimationConstant.PROP_STR_SCALE_X_Y;
import static com.lynx.tasm.animation.AnimationConstant.PROP_STR_SCALE_Y;
import static com.lynx.tasm.animation.AnimationConstant.PROP_STR_TOP;
import static com.lynx.tasm.animation.AnimationConstant.PROP_STR_WIDTH;
import static com.lynx.tasm.animation.AnimationConstant.PROP_TOP;
import static com.lynx.tasm.animation.AnimationConstant.PROP_TRANSFORM;
import static com.lynx.tasm.animation.AnimationConstant.PROP_VISIBILITY;
import static com.lynx.tasm.animation.AnimationConstant.PROP_WIDTH;

import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.PropsConstants;

public final class PropertyFactory {
  public static final int fromPropertyString(String name) {
    switch (name) {
      case PROP_STR_OPACITY:
        return PROP_OPACITY;
      case PROP_STR_SCALE_X:
        return PROP_SCALE_X;
      case PROP_STR_SCALE_Y:
        return PROP_SCALE_Y;
      case PROP_STR_SCALE_X_Y:
        return PROP_SCALE_X_Y;
      case PROP_STR_WIDTH:
        return PROP_WIDTH;
      case PROP_STR_HEIGHT:
        return PROP_HEIGHT;
      case PROP_STR_LEFT:
        return PROP_LEFT;
      case PROP_STR_TOP:
        return PROP_TOP;
      case PROP_STR_RIGHT:
        return PROP_RIGHT;
      case PROP_STR_BOTTOM:
        return PROP_BOTTOM;
      case PropsConstants.BACKGROUND_COLOR:
        return AnimationConstant.PROP_BACKGROUND_COLOR;
      case PropsConstants.VISIBILITY:
        return AnimationConstant.PROP_VISIBILITY;
      case PropsConstants.TRANSFORM:
        return AnimationConstant.PROP_TRANSFORM;
      default:
        LLog.DTHROW(new IllegalArgumentException("Unsupported animated property: " + name));
        return AnimationConstant.PROP_NONE;
    }
  }

  public static final String propertyToString(int prop) {
    switch (prop) {
      case PROP_OPACITY:
        return AnimationConstant.PROP_STR_OPACITY;
      case PROP_SCALE_X:
        return AnimationConstant.PROP_STR_SCALE_X;
      case PROP_SCALE_Y:
        return AnimationConstant.PROP_STR_SCALE_Y;
      case PROP_SCALE_X_Y:
        return PROP_STR_SCALE_X_Y;
      case PROP_WIDTH:
        return PROP_STR_WIDTH;
      case PROP_HEIGHT:
        return PROP_STR_HEIGHT;
      case PROP_LEFT:
        return PROP_STR_LEFT;
      case PROP_TOP:
        return PROP_STR_TOP;
      case PROP_RIGHT:
        return PROP_STR_RIGHT;
      case PROP_BOTTOM:
        return PROP_STR_BOTTOM;
      case PROP_BACKGROUND_COLOR:
        return PropsConstants.BACKGROUND_COLOR;
      case PROP_VISIBILITY:
        return PropsConstants.VISIBILITY;
      case PROP_TRANSFORM:
        return PropsConstants.TRANSFORM;
      default:
        LLog.DTHROW(new IllegalArgumentException("Unsupported animated property: " + prop));
        return "none";
    }
  }
}
