// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.component;

import androidx.annotation.Nullable;

/**
 * Deprecated, please use {@link #LynxTemplateResourceFetcher} instead.
 */
@Deprecated
public interface DynamicComponentFetcher {
  interface LoadedHandler {
    void onComponentLoaded(@Nullable byte[] data, @Nullable Throwable error);
  }

  void loadDynamicComponent(String url, LoadedHandler handler);
}
