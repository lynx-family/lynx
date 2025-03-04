// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;

public abstract class LynxResourceServiceCallback {
  /**
   * response callback
   * @param response
   */
  @AnyThread
  public void onResponse(@NonNull ILynxResourceServiceResponse response) {}
}
