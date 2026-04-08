// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;

/**
 * Callback for retrieving node metadata from a {@link LynxView}.
 */
public interface LynxNodeInfoCallback {
  void onResult(@NonNull LynxNodeInfoResult result);
}
