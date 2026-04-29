// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.blur;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public class TraceEventDef {
  public final static String BLUR_VIEW_CREATE_RENDER_SCRIPT = "BlurView.createRenderScript";
  public final static String BLUR_VIEW_RE_BLUR = "BlurView.reBlur";
  public final static String BLUR_VIEW_INNER_DRAW = "BlurView.innerDraw";
  public final static String BLUR_VIEW_GET_TARGET_PARENT = "BlurView.getTargetParent";
  public final static String BLUR_VIEW_CREATE_BITMAP = "BlurView.createBitmap";
  public final static String BLUR_VIEW_DESTORY = "BlurView.destroy";
  public final static String BLUR_VIEW_DRAW = "BlurView.draw";
  public final static String BLUR_VIEW_BLUR = "BlurView.blur";
  public final static String BLUR_VIEW_UPDATE_BLUR = "BlurView.updateBlur";
}
