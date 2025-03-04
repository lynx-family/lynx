// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import androidx.annotation.NonNull;
import com.lynx.tasm.INativeLibraryLoader;

public interface ILynxI18nService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxI18nService.class;
  }
  /**
   *
   * @param loader
   * System Loader or Custom Loader {@link
   * com.lynx.tasm.LynxEnv#initLibraryLoader(INativeLibraryLoader)}
   *
   * @return Indicates whether loading napi & icu so through LynxService was successful.
   */
  boolean loadLibrary(INativeLibraryLoader loader);

  /**
   * Currently, Intl.NumberFormat and Intl.DateTimeFormat will be mounted in NapiEnv. The NapiEnv
   * pointer of LepusNG is passed from the templateEntry decode stage, and the NapiEnv of JS is
   * passed from the PrepareJS stage of LynxRuntime.
   *
   * @param napiEnvPtr The NapiEnv pointer passed from the Native layer
   * @return Indicates whether the intl object has been successfully registered in primjs-icu.
   */
  boolean registerNapiEnv(long napiEnvPtr);
}
