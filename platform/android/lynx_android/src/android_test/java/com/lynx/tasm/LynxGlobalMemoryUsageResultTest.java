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
  public void mapsNativeGlobalAndInstanceSnapshots() {
    long[] globalValues = {784L, 60L, 6L, 600L, 24L, 100L};
    long[] instanceValues = {
        1L,
        187L,
        10L,
        1L,
        100L,
        7L,
        70L,
        2L,
        308L,
        20L,
        2L,
        200L,
        8L,
        80L,
        3L,
        359L,
        30L,
        3L,
        300L,
        9L,
        20L,
    };
    String[] instanceStrings = {
        "page-1",
        "url-1",
        "same-name",
        "page-2",
        "url-2",
        "same-name",
        "page-3",
        "url-3",
        "-1",
    };

    LynxGlobalMemoryUsageResult result = LynxGlobalMemoryUsageResult.fromNative(
        100L, 50L, 2000L, globalValues, instanceValues, instanceStrings);

    assertEquals(3, result.getExpectedInstanceCount());
    assertEquals(3, result.getCompletedInstanceCount());
    assertEquals(50L, result.getCollectionDurationMs());
    assertEquals(60L, result.getElementBytes());
    assertEquals(6L, result.getElementNodeCount());
    assertEquals(600L, result.getViewBytes());
    assertEquals(24L, result.getMainThreadRuntimeBytes());
    assertEquals(100L, result.getBackgroundThreadRuntimeBytes());
    assertEquals(784L, result.getTotalBytes());
    assertEquals(3, result.getInstances().get(0).getInstanceId());
    assertEquals(2, result.getInstances().get(1).getInstanceId());
    assertEquals(1, result.getInstances().get(2).getInstanceId());
    assertEquals("same-name", result.getInstances().get(2).getBtsRuntimeGroupId());
  }

  @Test
  public void fallsBackToEmptyResultForIncompleteGlobalSnapshot() {
    LynxGlobalMemoryUsageResult result = LynxGlobalMemoryUsageResult.fromNative(
        100L, 50L, 20L, new long[] {1L}, new long[] {1L, 2L}, new String[] {"page-1"});

    assertEquals(100L, result.getCollectionStartMs());
    assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
    assertEquals(50L, result.getCollectionDurationMs());
    assertEquals(20L, result.getCollectionTimeoutMs());
    assertEquals(0, result.getExpectedInstanceCount());
    assertEquals(0, result.getCompletedInstanceCount());
    assertEquals(0L, result.getTotalBytes());
    assertEquals(0L, result.getRatioToApp(), 0L);
    assertEquals(0L, result.getElementBytes());
    assertEquals(0L, result.getElementNodeCount());
    assertEquals(0L, result.getViewBytes());
    assertEquals(0L, result.getMainThreadRuntimeBytes());
    assertEquals(0L, result.getBackgroundThreadRuntimeBytes());
    assertEquals(0, result.getInstances().size());
  }

  @Test
  public void ignoresIncompleteNativeInstanceSnapshot() {
    long[] globalValues = {10L, 1L, 2L, 3L, 4L, 5L};
    long[] instanceValues = {1L, 10L, 1L, 2L, 3L, 4L, 5L};

    LynxGlobalMemoryUsageResult result = LynxGlobalMemoryUsageResult.fromNative(
        100L, 50L, 20L, globalValues, instanceValues, new String[] {"page-1", "url-1"});

    assertEquals(0, result.getExpectedInstanceCount());
    assertEquals(0, result.getCompletedInstanceCount());
    assertEquals(10L, result.getTotalBytes());
    assertEquals(0, result.getInstances().size());
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
