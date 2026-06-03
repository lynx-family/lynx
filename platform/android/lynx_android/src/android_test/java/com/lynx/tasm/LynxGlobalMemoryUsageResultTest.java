// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static com.lynx.tasm.LynxMemoryUsageTestUtils.createInstance;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import java.util.ArrayList;
import java.util.Arrays;
import org.junit.Test;

public class LynxGlobalMemoryUsageResultTest {
  @Test
  public void sortsInstancesByTotalBytesDescending() {
    LynxInstanceMemoryUsage smallInstance = createInstance(1, 50L, 50L, 0L, 0L, 0L, 0L, null);
    LynxInstanceMemoryUsage largeInstance = createInstance(2, 200L, 200L, 0L, 0L, 0L, 0L, null);
    LynxInstanceMemoryUsage mediumInstance = createInstance(3, 100L, 100L, 0L, 0L, 0L, 0L, null);

    LynxGlobalMemoryUsageResult result =
        LynxGlobalMemoryUsageResult.build(0L, LynxMemoryCollectionStatus.COMPLETED, 0L, 0L, 3, 0L,
            Arrays.asList(smallInstance, largeInstance, mediumInstance));

    assertEquals(largeInstance, result.getInstances().get(0));
    assertEquals(mediumInstance, result.getInstances().get(1));
    assertEquals(smallInstance, result.getInstances().get(2));
  }

  @Test
  public void aggregatesInstancesAndDeduplicatesSharedRuntime() {
    LynxInstanceMemoryUsage first = createInstance(1, 187L, 10L, 1L, 100L, 7L, 70L, "shared");
    LynxInstanceMemoryUsage second = createInstance(2, 308L, 20L, 2L, 200L, 8L, 80L, "shared");
    LynxInstanceMemoryUsage third = createInstance(3, 359L, 30L, 3L, 300L, 9L, 20L, "-1");

    LynxGlobalMemoryUsageResult result =
        LynxGlobalMemoryUsageResult.build(100L, LynxMemoryCollectionStatus.COMPLETED, 50L, 2000L, 4,
            0L, Arrays.asList(first, second, third));

    assertEquals(4, result.getExpectedInstanceCount());
    assertEquals(3, result.getCompletedInstanceCount());
    assertEquals(50L, result.getCollectionDurationMs());
    assertEquals(60L, result.getElementBytes());
    assertEquals(6L, result.getElementNodeCount());
    assertEquals(600L, result.getViewBytes());
    assertEquals(24L, result.getMainThreadRuntimeBytes());
    assertEquals(100L, result.getBackgroundThreadRuntimeBytes());
    assertEquals(784L, result.getTotalBytes());
    assertEquals(third, result.getInstances().get(0));
    assertEquals(second, result.getInstances().get(1));
    assertEquals(first, result.getInstances().get(2));
  }

  @Test
  public void copiesInstancesAndExposesImmutableList() {
    ArrayList<LynxInstanceMemoryUsage> instances = new ArrayList<>();
    LynxInstanceMemoryUsage first = createInstance(1, 200L, 100L, 0L, 50L, 25L, 25L, null);
    LynxInstanceMemoryUsage second = createInstance(2, 100L, 50L, 0L, 25L, 25L, 0L, null);
    instances.add(second);
    instances.add(first);

    LynxGlobalMemoryUsageResult result = LynxGlobalMemoryUsageResult.build(
        100L, LynxMemoryCollectionStatus.COMPLETED, 25L, 2000L, 2, 1200L, instances);
    instances.clear();

    assertEquals(2, result.getCompletedInstanceCount());
    assertEquals(300L, result.getTotalBytes());
    assertEquals(0.25D, result.getRatioToApp(), 0D);
    assertEquals(first, result.getInstances().get(0));
    assertEquals(second, result.getInstances().get(1));
    try {
      result.getInstances().add(createInstance(3, 1L, 1L, 0L, 0L, 0L, 0L, null));
      fail("Expected immutable instances list.");
    } catch (UnsupportedOperationException expected) {
      // Expected.
    }
  }
}
