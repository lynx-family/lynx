// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import androidx.annotation.NonNull;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;

public class LynxMemoryUsageQueryTest {
  @Test
  public void queryGlobalMemoryUsageReturnsEmptyCompletedResultAsync() throws Exception {
    LynxEnv env = LynxEnv.inst();
    boolean wasNativeLibraryLoaded = env.mIsNativeLibraryLoaded;
    env.setNativeLibraryLoaded(false);
    CountDownLatch latch = new CountDownLatch(1);
    AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();
    long beforeQueryMs = System.currentTimeMillis();

    try {
      LynxMemoryUsageQuery.inst().queryLynxGlobalMemoryUsageAsync(
          new LynxGlobalMemoryUsageCallback() {
            @Override
            public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
              resultRef.set(result);
              latch.countDown();
            }
          });

      assertTrue(latch.await(1, TimeUnit.SECONDS));
    } finally {
      env.setNativeLibraryLoaded(wasNativeLibraryLoaded);
    }
    LynxGlobalMemoryUsageResult result = resultRef.get();
    assertNotNull(result);
    assertTrue(result.getCollectionStartMs() >= beforeQueryMs);
    assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
    assertTrue(result.getCollectionDurationMs() >= 0L);
    assertEquals(2000L, result.getCollectionTimeoutMs());
    assertEquals(0, result.getExpectedInstanceCount());
    assertEquals(0, result.getCompletedInstanceCount());
    assertEquals(0L, result.getTotalBytes());
    assertTrue(result.getAppBytes() >= 0L);
    assertEquals(0D, result.getRatioToApp(), 0D);
    assertEquals(0L, result.getElementBytes());
    assertEquals(0L, result.getElementNodeCount());
    assertEquals(0L, result.getViewBytes());
    assertEquals(0L, result.getMainThreadRuntimeBytes());
    assertEquals(0L, result.getBackgroundThreadRuntimeBytes());
    assertTrue(result.getInstances().isEmpty());
  }

  @Test
  public void queryGlobalMemoryUsageIgnoresNullCallback() {
    LynxMemoryUsageQuery.inst().queryLynxGlobalMemoryUsageAsync(null);
  }
}
