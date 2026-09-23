// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import java.util.concurrent.atomic.AtomicReference;

final class LynxMemoryUsageTestUtils {
  private LynxMemoryUsageTestUtils() {}

  static LynxInstanceMemoryUsage createInstance(int instanceId, long totalBytes, long elementBytes,
      long elementNodeCount, long viewBytes, long mainThreadRuntimeBytes,
      long backgroundThreadRuntimeBytes, String groupId) {
    return new LynxInstanceMemoryUsage(instanceId, "page-" + instanceId, "url-" + instanceId,
        totalBytes, elementBytes, elementNodeCount, viewBytes, null, mainThreadRuntimeBytes,
        backgroundThreadRuntimeBytes, groupId);
  }

  static LynxGlobalMemoryUsageCallback createResultCallback(
      @NonNull AtomicReference<LynxGlobalMemoryUsageResult> resultRef) {
    return new LynxGlobalMemoryUsageCallback() {
      @Override
      public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
        resultRef.set(result);
      }
    };
  }

  static void runWithNativeLibraryLoaded(@NonNull ThrowingRunnable runnable) throws Exception {
    LynxEnv env = LynxEnv.inst();
    boolean wasNativeLibraryLoaded = env.mIsNativeLibraryLoaded;
    env.setNativeLibraryLoaded(true);
    try {
      runnable.run();
    } finally {
      env.setNativeLibraryLoaded(wasNativeLibraryLoaded);
    }
  }

  interface ThrowingRunnable {
    void run() throws Exception;
  }
}
