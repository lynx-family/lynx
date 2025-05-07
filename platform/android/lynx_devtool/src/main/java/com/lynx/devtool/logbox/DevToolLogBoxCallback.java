// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.logbox;

public interface DevToolLogBoxCallback {
  default void onSuccess(String data) {}
  default void onFailure(String reason) {}
}
