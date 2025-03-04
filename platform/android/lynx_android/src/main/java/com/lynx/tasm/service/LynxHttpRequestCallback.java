// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import com.lynx.jsbridge.network.HttpResponse;

public abstract class LynxHttpRequestCallback {
  /**
   * response callback
   * @param response
   */
  @AnyThread
  public void invoke(@NonNull HttpResponse response) {}
}
