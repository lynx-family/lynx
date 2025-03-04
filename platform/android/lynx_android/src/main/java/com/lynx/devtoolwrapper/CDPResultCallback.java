// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

/**
 * Interface for handling results from CDP (Chrome DevTools Protocol) operations.
 */
public interface CDPResultCallback {
  /**
   * Called when a result is available from a CDP operation.
   *
   * @param result The result of the CDP operation as a JSON-formatted string.
   */
  void onResult(String result);
}
