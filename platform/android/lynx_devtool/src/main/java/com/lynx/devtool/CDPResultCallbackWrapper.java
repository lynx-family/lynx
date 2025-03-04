// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import androidx.annotation.Keep;
import com.lynx.devtoolwrapper.CDPResultCallback;
import com.lynx.tasm.base.CalledByNative;

@Keep
public class CDPResultCallbackWrapper {
  public CDPResultCallbackWrapper(CDPResultCallback callback) {
    this.callback = callback;
  }

  @CalledByNative
  public void onResult(String message) {
    callback.onResult(message);
  }

  private CDPResultCallback callback;
}
