// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

/**
 * Render templates without relying on LynxView/Context, only provide necessary rendering interfaces
 */
public interface ILynxEngine {
  void loadTemplate(final LynxLoadMeta metaData);

  void updateMetaData(LynxUpdateMeta meta);

  void updateViewport(int widthMeasureSpec, int heightMeasureSpec);

  void destroy();
}
