// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import androidx.annotation.NonNull;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.LynxContext;

/*
 * This is a experimental API, it is unstable and may break at any time.
 */
public interface ILynxExtensionService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxExtensionService.class;
  }
  void onLynxEnvSetup();

  void onLynxViewSetup(LynxContext context, LynxGroup group, BehaviorRegistry behaviorRegistry);

  void onLynxViewDestroy(LynxContext context);
}
