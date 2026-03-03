// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

public class WebAssemblyBridge {
  public static void initWasmRegisterFunc(long funcPtr) {
    nativeInitWasm(funcPtr);
  }

  public static boolean initWasm() {
    boolean isHasInit = false;
    long funcPtr = WebAssemblyReflect.getWasmRegister();
    if (funcPtr != 0) {
      initWasmRegisterFunc(funcPtr);
      isHasInit = true;
    }
    return isHasInit;
  }

  private static native void nativeInitWasm(long funcPtr);
}
