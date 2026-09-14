// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import com.lynx.tasm.base.CalledByNative;

public interface LynxJavaScriptExecutionCallback {
  int SUCCESS = 0;
  int UNSUPPORTED_ENGINE = 1;
  int NOT_READY = 2;
  int DESTROYED = 3;

  @CalledByNative void onResult(int status, String stack);
}
