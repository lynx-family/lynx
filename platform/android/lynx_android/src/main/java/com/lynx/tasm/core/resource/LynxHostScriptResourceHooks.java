// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.core.resource;

import androidx.annotation.Nullable;
import com.lynx.tasm.base.CalledByNative;

/** Bundle fetch decisions provided by the optional Host Script library. */
final class LynxHostScriptResourceHooks {
  private static volatile boolean sAvailable;

  private LynxHostScriptResourceHooks() {}

  @CalledByNative
  private static void setAvailable() {
    sAvailable = true;
  }

  static boolean hasHandler(long viewId) {
    return sAvailable && viewId >= 0 && nativeHasHandler(viewId);
  }

  private static native boolean nativeHasHandler(long viewId);

  // On UI. Returns a redirect String, mock byte[], or null to continue the original fetch.
  @Nullable static native Object nativeDispatch(long viewId, byte[] url);
}
