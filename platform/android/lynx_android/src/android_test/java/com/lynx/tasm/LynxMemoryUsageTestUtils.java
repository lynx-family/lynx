// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.assertFalse;

import androidx.annotation.NonNull;
import com.lynx.tasm.base.LynxConsumer;
import java.util.ArrayList;
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

  static LynxMemoryUsageFetcher createNoOpFetcher() {
    return new LynxMemoryUsageFetcher() {
      @Override
      void queryMemoryUsageAsync(@NonNull LynxConsumer<LynxInstanceMemoryUsage> callback) {}
    };
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

  static class TestCollector extends LynxGlobalMemoryUsageCollector {
    private final ArrayList<Runnable> mReportTasks = new ArrayList<>();
    private final ArrayList<Runnable> mDelayedTasks = new ArrayList<>();
    private final ArrayList<Long> mDelayedMsList = new ArrayList<>();

    @Override
    void runOnReportThread(@NonNull Runnable runnable) {
      mReportTasks.add(runnable);
    }

    @Override
    void delayRunOnReportThread(@NonNull Runnable runnable, long delayMs) {
      mDelayedTasks.add(runnable);
      mDelayedMsList.add(delayMs);
    }

    void runNextReportTask() {
      assertFalse(mReportTasks.isEmpty());
      mReportTasks.remove(0).run();
    }

    void runNextDelayedTask() {
      assertFalse(mDelayedTasks.isEmpty());
      mDelayedMsList.remove(0);
      mDelayedTasks.remove(0).run();
    }

    int getReportTaskCount() {
      return mReportTasks.size();
    }

    int getDelayedTaskCount() {
      return mDelayedTasks.size();
    }

    long getDelayedMs(int index) {
      return mDelayedMsList.get(index);
    }
  }
}
