// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

final class FSPReportDispatcherTestHelper {
  private static boolean sRegistered;

  private FSPReportDispatcherTestHelper() {}

  static synchronized int getActiveGlobalRefCount() {
    if (!sRegistered) {
      if (!registerJNI()) {
        throw new IllegalStateException("Failed to register FSP dispatcher test JNI.");
      }
      sRegistered = true;
    }
    return nativeGetActiveGlobalRefCount();
  }

  private static native boolean registerJNI();

  private static native int nativeGetActiveGlobalRefCount();
}
