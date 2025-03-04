// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll;

import android.content.Context;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.behavior.ui.view.UISimpleView;

public class UIBounceView extends UISimpleView<AndroidView> {
  public static final int RIGHT = 0;
  public static final int LEFT = 1;
  public static final int TOP = 2;
  public static final int BOTTOM = 3;

  public int mDirection = RIGHT;

  public UIBounceView(LynxContext context) {
    super(context);
  }

  public AndroidView createView(Context context) {
    return new AndroidView(context);
  }

  @LynxProp(name = "direction", customType = "right")
  public void setDirection(Dynamic direction) {
    if (direction.getType() == ReadableType.String) {
      String directionStr = direction.asString();
      if (directionStr.equals("right")) {
        mDirection = RIGHT;
      } else if (directionStr.equals("left")) {
        mDirection = LEFT;
      } else if (directionStr.equals("top")) {
        mDirection = TOP;
      } else if (directionStr.equals("bottom")) {
        mDirection = BOTTOM;
      }
    }
  }
}
