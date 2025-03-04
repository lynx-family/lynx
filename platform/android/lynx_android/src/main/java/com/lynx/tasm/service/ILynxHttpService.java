// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.NonNull;
import com.lynx.jsbridge.network.HttpRequest;

public interface ILynxHttpService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxHttpService.class;
  }

  void request(@NonNull HttpRequest request, @NonNull final LynxHttpRequestCallback callback);
}
