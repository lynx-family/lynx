// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.utils;

// Uses for auto register props holder, don't invoke this class directly!
public class PropsHolderAutoRegister {
  public static boolean sHasRegistered;

  static {
    sHasRegistered = false;
  }

  public static void registerLynxUISetter(LynxUISetter settable) {
    PropsUpdater.registerSetter(settable);
  }

  public static void registerShadowNodeSetter(ShadowNodeSetter settable) {
    PropsUpdater.registerSetter(settable);
  }

  public static void init() {
    if (!sHasRegistered) {
      sHasRegistered = true;
    }
  }
}
