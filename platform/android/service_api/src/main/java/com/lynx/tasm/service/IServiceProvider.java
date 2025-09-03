// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import android.content.Context;
import androidx.annotation.Keep;

@Keep
public interface IServiceProvider {
  /**
   * Provide a class as the key of the service to be registered.
   * <p>
   * TODO @dangyingkai remove this after service discovery is supported
   */
  Class<? extends IServiceProvider> getServiceClass();

  /**
   * Service initialization timing
   * <p>
   * Only guarantees to be called before the first retrieval,
   * does not guarantee any specific timing.
   */
  default void onInitialize(Context context) {}
}
