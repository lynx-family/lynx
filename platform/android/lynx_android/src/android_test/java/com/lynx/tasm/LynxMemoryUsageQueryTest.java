// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.*;

import androidx.annotation.NonNull;
import java.util.Arrays;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;

public class LynxMemoryUsageQueryTest {
  @Test
  public void queryGlobalMemoryUsageReturnsEmptyCompletedResultAsync() throws Exception {
    CountDownLatch latch = new CountDownLatch(1);
    AtomicReference<LynxGlobalMemoryUsageResult> resultRef = new AtomicReference<>();
    long beforeQueryMs = System.currentTimeMillis();

    LynxMemoryUsageQuery.inst().queryLynxGlobalMemoryUsageAsync(
        new LynxGlobalMemoryUsageCallback() {
          @Override
          public void onResult(@NonNull LynxGlobalMemoryUsageResult result) {
            resultRef.set(result);
            latch.countDown();
          }
        });

    assertTrue(latch.await(1, TimeUnit.SECONDS));
    LynxGlobalMemoryUsageResult result = resultRef.get();
    assertNotNull(result);
    assertTrue(result.getCollectionStartMs() >= beforeQueryMs);
    assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
    assertEquals(0L, result.getCollectionDurationMs());
    assertEquals(0L, result.getCollectionTimeoutMs());
    assertEquals(0, result.getExpectedInstanceCount());
    assertEquals(0, result.getCompletedInstanceCount());
    assertEquals(0L, result.getTotalBytes());
    assertEquals(0L, result.getAppBytes());
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

  @Test
  public void globalMemoryUsageResultSortsInstancesByTotalBytesDescending() {
    LynxInstanceMemoryUsage smallInstance = new LynxInstanceMemoryUsage(
        1, "page-small", "url-small", 50L, 0L, 0L, 0L, null, 0L, 0L, "group-small");
    LynxInstanceMemoryUsage largeInstance = new LynxInstanceMemoryUsage(
        2, "page-large", "url-large", 200L, 0L, 0L, 0L, null, 0L, 0L, "group-large");
    LynxInstanceMemoryUsage mediumInstance = new LynxInstanceMemoryUsage(
        3, "page-medium", "url-medium", 100L, 0L, 0L, 0L, null, 0L, 0L, "group-medium");

    LynxGlobalMemoryUsageResult result = new LynxGlobalMemoryUsageResult(0L,
        LynxMemoryCollectionStatus.COMPLETED, 0L, 0L, 3, 3, 350L, 0L, 0D, 0L, 0L, 0L, 0L, 0L,
        Arrays.asList(smallInstance, largeInstance, mediumInstance));

    assertEquals(largeInstance, result.getInstances().get(0));
    assertEquals(mediumInstance, result.getInstances().get(1));
    assertEquals(smallInstance, result.getInstances().get(2));
  }
}
