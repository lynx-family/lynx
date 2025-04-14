// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public interface RuntimeFullLifecycleListener extends RuntimeLifecycleListener {
  void onRuntimeCreate(long vsyncMonitorPtr);

  void onRuntimeInit(long runtimeId);

  void onAppEnterBackground();

  void onAppEnterForeground();
}
