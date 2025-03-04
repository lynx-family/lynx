// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.network;

public interface DownloadCallback {
  default void onResponse(int status, int contentLength) {}

  default void onData(byte[] bytes, int length) {}

  default void onFailure(String reason) {}
}
