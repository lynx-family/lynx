// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.fluency;

import static com.lynx.tasm.LynxEnvKey.ENABLE_FLUENCY_TRACE;

import com.lynx.tasm.LynxEnv;
import java.util.concurrent.atomic.AtomicBoolean;

public class FluencySample {
  private static final AtomicBoolean sCheckUpdate = new AtomicBoolean(true);
  private static boolean sEnable = false;
  private static boolean sForceEnable = false;

  public static boolean isEnable() {
    if (sForceEnable) {
      return true;
    }

    if (sCheckUpdate.compareAndSet(true, false)) {
      sEnable = LynxEnv.getBooleanFromExternalEnv(ENABLE_FLUENCY_TRACE, false);
    }
    return sEnable;
  }

  public static void setEnable(boolean enable) {
    sForceEnable = enable;
    nativeSetFluencySample(enable);
  }

  public static void needCheckUpdate() {
    sCheckUpdate.set(true);
    nativeNeedCheckFluencyEnable();
  }

  private static native void nativeSetFluencySample(boolean enable);
  private static native void nativeNeedCheckFluencyEnable();
}
