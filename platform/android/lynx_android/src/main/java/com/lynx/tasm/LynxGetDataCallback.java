// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import com.lynx.react.bridge.JavaOnlyMap;

/**
 * Asynchronous data retrieval callback;
 */
public interface LynxGetDataCallback {
  void onSuccess(JavaOnlyMap data);

  void onFail(String msg);
}
