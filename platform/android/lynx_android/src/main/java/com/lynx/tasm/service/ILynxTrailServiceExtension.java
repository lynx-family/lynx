// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import com.lynx.tasm.LynxViewBuilder;

@Keep
public interface ILynxTrailServiceExtension extends IServiceExtension {
  /**
   * Parse lynx view builder
   * @param builder view builder
   */
  void parseLynxViewBuilder(@NonNull LynxViewBuilder builder);
}
