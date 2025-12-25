// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import com.lynx.tasm.behavior.ClayRenderMode;
import com.lynx.tasm.behavior.ILynxUIRenderer;

public interface IUIRendererCreator {
  /**
   * Provide the implementation of platform layer UI-related components/method
   *
   * @return UIRender
   */
  ILynxUIRenderer createLynxUIRender();

  public default void setLowEndImageTextureCacheMaxLimit(int limit) {}

  public default void setImageTextureCacheMaxLimit(int limit) {}

  public default void setUseTextureViewInClayMode(boolean useTextureView) {}

  public default void setEnableGLFunctorInClayMode(boolean enable) {}

  public default void setClayRenderMode(ClayRenderMode mode) {}

  public default void setEnableDelegateInClayMode(boolean enable) {}

  public default void setEnableClayRecycleEngine(boolean enable) {}

  public default void setEnableClayCompatMode(boolean enable) {}
}
