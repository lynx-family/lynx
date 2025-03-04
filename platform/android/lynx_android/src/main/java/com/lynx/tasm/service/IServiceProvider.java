// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.NonNull;

public interface IServiceProvider {
  // Provide the class of the service to be registered.
  @NonNull Class<? extends IServiceProvider> getServiceClass();
}
