// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import com.lynx.tasm.service.IServiceProvider;

/*
 * Developers are required to introduce the corresponding C/C++ library, overload the function
 * ILynxTraceService::getDefaultTraceFunction, and return the address of the C++ layer log
 * processing function via this function.
 * */

@Keep
public interface ILynxTraceService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxTraceService.class;
  }

  /*
   * Return the address of the C/C++ trace function.
   * */
  long getDefaultTraceFunction();
}
