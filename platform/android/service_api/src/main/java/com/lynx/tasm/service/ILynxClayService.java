// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.Map;

@Keep
public interface ILynxClayService extends IServiceProvider {
  interface PreWarmEventReporter {
    void reportPreWarm(@NonNull String eventName, @NonNull Map<String, Object> props);
  }

  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxClayService.class;
  }

  default void setPreWarmEventReporter(@Nullable PreWarmEventReporter reporter) {}
}
