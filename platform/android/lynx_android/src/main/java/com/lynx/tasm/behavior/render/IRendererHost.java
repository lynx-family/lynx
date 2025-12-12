// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.view.ViewGroup;
public interface IRendererHost {
  void setRenderer(Renderer renderer);
  Renderer getRenderer();
  ViewGroup getView();
  Renderer createRenderer(PlatformRendererContext platformRendererContext, int sign);
}
