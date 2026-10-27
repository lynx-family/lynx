// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax;

import android.content.Context;
import android.view.View;
import com.lynx.animax.ui.AnimaXContainerView;
import com.lynx.tasm.behavior.render.IRendererHost;
import com.lynx.tasm.behavior.render.PlatformRendererContext;
import com.lynx.tasm.behavior.render.Renderer;

// Keep FrameLayout measurement and layout: the player Views are native children,
// not renderer hosts, and must fill the container's content bounds.
final class LynxAnimaXContainerView extends AnimaXContainerView implements IRendererHost {
  private Renderer mRenderer;

  LynxAnimaXContainerView(Context context) {
    super(context);
  }

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

  @Override
  public Renderer createRenderer(PlatformRendererContext context, int sign) {
    // AnimaX is a leaf: its native player draws without Renderer draw callbacks.
    return new Renderer(context, sign);
  }
}
