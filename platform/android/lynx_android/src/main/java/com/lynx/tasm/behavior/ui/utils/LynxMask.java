// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.utils;

import com.lynx.tasm.behavior.LynxContext;

public class LynxMask extends LynxDrawableManager<MaskDrawable> {
  public LynxMask(LynxContext context) {
    super(context);
  }

  @Override
  protected MaskDrawable createLayerDrawable() {
    return new MaskDrawable(mContext, mFontSize);
  }
}
