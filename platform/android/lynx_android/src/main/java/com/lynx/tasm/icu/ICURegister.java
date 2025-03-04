// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.icu;

import androidx.annotation.NonNull;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.service.ILynxI18nService;
import com.lynx.tasm.service.LynxServiceCenter;

/**
 *
 */
public class ICURegister {
  /**
   * Indicates whether loading napi & icu so through LynxService was successful.
   * Move the flag indicating whether the loading was successful to the front, to avoid calling
   * register through LynxService reflection if loading the so fails.
   */
  private static volatile boolean mLibraryLoaded = false;

  public static synchronized void loadLibrary(@NonNull INativeLibraryLoader loader) {
    if (mLibraryLoaded) {
      return;
    }

    ILynxI18nService service = LynxServiceCenter.inst().getService(ILynxI18nService.class);
    if (service != null) {
      mLibraryLoaded = service.loadLibrary(loader);
    }
  }

  @CalledByNative
  private static synchronized boolean register(long napiEnvPtr) {
    if (mLibraryLoaded) {
      ILynxI18nService service = LynxServiceCenter.inst().getService(ILynxI18nService.class);
      if (service != null) {
        return service.registerNapiEnv(napiEnvPtr);
      }
    }
    return false;
  }
}
