// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import com.lynx.tasm.base.LLog;
import java.lang.reflect.Method;

public class WebAssemblyReflect {
  public static long getWasmRegister() {
    long funcPtr = 0;
    try {
      Class<?> wasmClazz = Class.forName("com.lynx.primjs.wasm.RegisterWebAssembly");
      Method method = wasmClazz.getMethod("registerWebAssembly");
      funcPtr = (long) method.invoke(null);
    } catch (Exception e) {
      LLog.e("lynx",
          "No webassembly found in the host [ " + e.getMessage() + ", " + e.getCause() + " ]");
    }
    return funcPtr;
  }
}
