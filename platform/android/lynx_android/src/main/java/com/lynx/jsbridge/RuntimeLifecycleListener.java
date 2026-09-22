// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

public interface RuntimeLifecycleListener {
  /**
   * Called when the runtime is attached. Kept for listeners using the original callback signature.
   *
   * @param napiEnv the attached NAPI environment
   */
  default void onRuntimeAttach(long napiEnv) {}

  /**
   * Called when the runtime is attached, with its runtime type.
   * The default implementation forwards to the original callback for compatibility with existing
   * listeners.
   *
   * @param napiEnv the attached NAPI environment
   * @param runtimeType the JavaScript runtime type
   */
  default void onRuntimeAttach(long napiEnv, String runtimeType) {
    onRuntimeAttach(napiEnv);
  }

  void onRuntimeDetach();
}
