// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static com.lynx.tasm.LynxMemoryUsageTestUtils.createInstance;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
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
    assertEquals(0.0, result.getRatioToApp(), 0.0);
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
        1000L,
        2L,
        308L,
        20L,
        2L,
        200L,
        8L,
        80L,
        1000L,
        3L,
        359L,
        30L,
        3L,
        300L,
        9L,
        20L,
        20L,
    };
    String[] instanceStrings = {
        "page-1",
        "url-1",
        "same-name",
        "quickjs",
        "page-2",
        "url-2",
        "same-name",
        "quickjs",
        "page-3",
        "url-3",
        "-1",
        "jsc",
    };

    LynxGlobalMemoryUsageResult result = LynxGlobalMemoryUsageResult.fromNative(100L, 50L, 2000L, 0,
        globalValues, instanceValues, instanceStrings, new long[0], new String[0]);

    assertEquals(3, result.getExpectedInstanceCount());
    assertTrue(result.isMemoryMonitoringEnabled());
    assertEquals(3, result.getCompletedInstanceCount());
    assertEquals(50L, result.getCollectionDurationMs());
    assertEquals(60L, result.getElementBytes());
    assertEquals(6L, result.getElementNodeCount());
    assertEquals(600L, result.getViewBytes());
    assertEquals(24L, result.getMainThreadRuntimeBytes());
    assertEquals(100L, result.getBackgroundThreadRuntimeBytes());
    assertEquals(784L, result.getTotalBytes());
    assertEquals(
        result.getAppBytes() > 0 ? 784.0 / result.getAppBytes() : 0.0, result.getRatioToApp(), 0.0);
    assertEquals(3, result.getInstances().get(0).getInstanceId());
    assertEquals(2, result.getInstances().get(1).getInstanceId());
    assertEquals(1, result.getInstances().get(2).getInstanceId());
    assertEquals("same-name", result.getInstances().get(2).getBtsRuntimeGroupId());
    assertEquals("quickjs", result.getInstances().get(2).getBtsRuntimeType());
    assertEquals(1000L, result.getInstances().get(2).getBtsRuntimeTotalBytes());
    assertTrue(result.getInstances().get(2).getViewDetail().isEmpty());
  }

  @Test
  public void fallsBackToEmptyResultForIncompleteGlobalSnapshot() {
    LynxGlobalMemoryUsageResult result = LynxGlobalMemoryUsageResult.fromNative(100L, 50L, 20L, 0,
        new long[] {1L}, new long[] {1L, 2L}, new String[] {"page-1"}, new long[0], new String[0]);

    assertEquals(100L, result.getCollectionStartMs());
    assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
    assertEquals(50L, result.getCollectionDurationMs());
    assertEquals(20L, result.getCollectionTimeoutMs());
    assertEquals(0, result.getExpectedInstanceCount());
    assertEquals(0, result.getCompletedInstanceCount());
    assertEquals(0L, result.getTotalBytes());
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
    long[] instanceValues = {1L, 10L, 1L, 2L, 3L, 4L, 5L, 50L};

    LynxGlobalMemoryUsageResult result = LynxGlobalMemoryUsageResult.fromNative(100L, 50L, 20L, 0,
        globalValues, instanceValues, new String[] {"page-1", "url-1"}, new long[0], new String[0]);

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
    assertEquals(0.25, result.getRatioToApp(), 0.0);
    assertEquals(first, result.getInstances().get(0));
    assertEquals(second, result.getInstances().get(1));
    try {
      result.getInstances().add(createInstance(3, 1L, 1L, 0L, 0L, 0L, 0L, null));
      fail("Expected immutable instances list.");
    } catch (UnsupportedOperationException expected) {
      // Expected.
    }
  }

  @Test
  public void mapsDestroyedInstancesWithoutPageMetrics() {
    long[] destroyedValues = {11, 900, 500, 12, 900, 400, 21, 600, 300};
    String[] destroyedStrings = {"page-11", "https://host/page?route=detail#original😀",
        "same-name", "quickjs", "page-12", "url-12", "same-name", "quickjs", "page-21", "url-21",
        "same-name", "quickjs"};
    LynxGlobalMemoryUsageResult result = LynxGlobalMemoryUsageResult.fromNative(100, 50, 2000, 0,
        new long[6], new long[0], new String[0], destroyedValues, destroyedStrings);
    assertEquals(3, result.getDestroyedInstances().size());
    LynxInstanceMemoryUsage first = result.getDestroyedInstances().get(0);
    assertEquals(11, first.getInstanceId());
    assertEquals("page-11", first.getPageId());
    assertEquals(destroyedStrings[1], first.getUrl());
    assertEquals("quickjs", first.getBtsRuntimeType());
    assertEquals(900, first.getBtsRuntimeTotalBytes());
    assertEquals(500, first.getBackgroundThreadRuntimeResidualBytes());
    assertEquals(0, first.getTotalBytes());
    assertEquals(0, first.getElementBytes());
    assertEquals(0, first.getElementNodeCount());
    assertEquals(0, first.getViewBytes());
    assertTrue(first.getViewDetail().isEmpty());
    assertEquals(0, first.getMainThreadRuntimeBytes());
    assertEquals(0, first.getBackgroundThreadRuntimeBytes());
    assertEquals(21, result.getDestroyedInstances().get(2).getInstanceId());
    assertEquals(0, result.getCompletedInstanceCount());
    try {
      result.getDestroyedInstances().clear();
      fail("Expected immutable destroyed instances list.");
    } catch (UnsupportedOperationException expected) {
      // Expected.
    }
  }

  @Test
  public void disabledMonitoringPreservesOnDemandSnapshot() {
    LynxGlobalMemoryUsageResult result = LynxGlobalMemoryUsageResult.fromNative(100, 50, 2000, 1,
        new long[] {130, 10, 1, 0, 20, 100}, new long[] {1, 30, 10, 1, 0, 20, 0, 100},
        new String[] {"page", "url", "group", "quickjs"}, new long[0], new String[0]);
    assertEquals(LynxMemoryCollectionStatus.COMPLETED, result.getCollectionStatus());
    assertEquals("", result.getErrorMessage());
    assertFalse(result.isMemoryMonitoringEnabled());
    assertEquals(130, result.getTotalBytes());
    assertEquals(100, result.getBackgroundThreadRuntimeBytes());
    assertEquals(1, result.getCompletedInstanceCount());
    assertEquals(100, result.getInstances().get(0).getBtsRuntimeTotalBytes());
    assertEquals(0, result.getInstances().get(0).getBackgroundThreadRuntimeBytes());
    assertTrue(result.getDestroyedInstances().isEmpty());
  }
}
