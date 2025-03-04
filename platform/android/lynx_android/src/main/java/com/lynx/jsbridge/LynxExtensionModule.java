// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.LynxContext;

/*
 * This is a experimental API, it is unstable and may break at any time.
 */
public abstract class LynxExtensionModule {
  protected LynxContext mContext;
  protected LynxGroup mGroup;

  public LynxExtensionModule(
      LynxContext context, LynxGroup group, BehaviorRegistry behaviorRegistry) {
    mContext = context;
    mGroup = group;
  }

  public abstract long getExtensionDelegatePtr();
  public abstract void setUp();
  public abstract void destroy();
}
