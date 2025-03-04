// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.graphics.Color;
import androidx.annotation.ColorInt;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.shadow.ShadowNode;

/**
 * Platform ShadowNode to hold ::selection style information
 *   Currently, only background-color is supported
 */
public class TextSelectionShadowNode extends ShadowNode {
  @ColorInt private int mBackgroundColor = 0;

  @Override
  public boolean isVirtual() {
    return true;
  }

  @LynxProp(name = PropsConstants.BACKGROUND_COLOR, defaultInt = Color.TRANSPARENT)
  public void setBackgroundColor(int color) {
    mBackgroundColor = color;
  }

  public int getBackgroundColor() {
    return mBackgroundColor;
  }
}
