// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.provider;

import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;

/**
 * Starndard Lynx Resource Callback
 */
public abstract class LynxResourceCallback<C> {
  @AnyThread
  public void onResponse(@NonNull LynxResourceResponse<C> response) {}
}
