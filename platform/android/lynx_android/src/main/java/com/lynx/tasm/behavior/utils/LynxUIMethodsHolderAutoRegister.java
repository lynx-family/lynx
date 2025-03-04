// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.utils;

public class LynxUIMethodsHolderAutoRegister {
  public static boolean sHasRegistered;

  static {
    sHasRegistered = false;
  }

  public static void registerLynxUIMethodInvoker(LynxUIMethodInvoker methodInvoker) {
    LynxUIMethodsExecutor.registerMethodInvoker(methodInvoker);
  }

  public static void init() {
    if (!sHasRegistered) {
      sHasRegistered = true;
    }
  }
}
