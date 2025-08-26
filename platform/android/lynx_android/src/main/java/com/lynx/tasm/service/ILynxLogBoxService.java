// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import com.lynx.devtoolwrapper.ILynxLogBox;
import com.lynx.devtoolwrapper.LynxDevtool;

@Keep
public interface ILynxLogBoxService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxLogBoxService.class;
  }
  ILynxLogBox createLogBox(@NonNull LynxDevtool devtool);
}
