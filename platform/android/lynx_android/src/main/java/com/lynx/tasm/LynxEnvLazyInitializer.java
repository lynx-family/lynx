// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

public class LynxEnvLazyInitializer {
  static Initializer sInitializer;

  public static void setLazyInitializer(Initializer initializer) {
    sInitializer = initializer;
  }

  public static Initializer getsInitializer() {
    return sInitializer;
  }

  public interface Initializer {
    void init();
  }
}
