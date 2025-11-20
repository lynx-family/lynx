// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.graphics.Rect;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.performance.PerformanceController;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class FSPTracerTest {
  private FSPTracer tracer;
  private FSPConfig config;
  private int containerWidth = 512;
  private int containerHeight = 1024;

  @Mock private PerformanceController mockPerfController;

  @Mock private IMeaningfulContentSnapshotCaptureHandler mockCaptureHandler;

  @Before
  public void setUp() {
    setField(LynxEnv.class, LynxEnv.inst(), "mEnableFSP", true);
    // Create tracer with mocked performance controller
    tracer = new FSPTracer(mockPerfController);
    // Setup config
    config = (FSPConfig) getField(FSPTracer.class, tracer, "mConfig");
    config.parse();
    config.minContentFillPercentageX = 30;
    config.minContentFillPercentageY = 30;
    config.minContentFillPercentageTotalArea = 30;
    config.acceptablePixelDiffPerSec = 10;
    config.acceptableAreaDiffPerSec = 100;
    config.minDiffIntervalMs = 300;
    config.hardTimeoutMs = 100000;
    config.snapshotIntervalMs = 17;
  }

  @Test
  public void testConstructor() {
    assertNotNull(tracer);
  }

  @Test
  public void testOnCaptureSnapshot_NotValuableSnapshot() {
    // Create a snapshot with minimal content (not valuable)
    FSPSnapshot snapshot = new FSPSnapshot(containerWidth, containerHeight, 1000000);
    snapshot.fillContentToSnapshot(true, new Rect(0, 0, 10, 10), 1000000);
    // Call the method directly
    tracer.onCaptureSnapshot(snapshot);
    // Verify perfController was not called
    verify(mockPerfController, never()).setFSPTimingInfo(anyLong(), anyMap());
  }

  @Test
  public void testOnCaptureSnapshot_ValuableSnapshotWithoutPrevious() {
    // Create a valuable snapshot
    FSPSnapshot snapshot = createValuableSnapshot(1000000);
    // Call the method directly
    tracer.onCaptureSnapshot(snapshot);
    // Verify perfController was not called yet (need a stable pair)
    verify(mockPerfController, never()).setFSPTimingInfo(anyLong(), anyMap());
  }

  @Test
  public void testOnCaptureSnapshot_ValuableAndStableSnapshots() {
    setField(FSPTracer.class, tracer, "mIsRunning", new AtomicBoolean(true));
    // Create two valuable and stable snapshots
    // Create two stable snapshots
    long timestamp = 1000000;
    FSPSnapshot firstSnapshot = createValuableSnapshot(timestamp);
    FSPSnapshot secondSnapshot = createValuableSnapshot(
        (timestamp + (long) ((config.minDiffIntervalMs + 1) * 1000))); // 10ms later
    // Call onCaptureSnapshot twice
    tracer.onCaptureSnapshot(firstSnapshot);
    tracer.onCaptureSnapshot(secondSnapshot);
    // Verify perfController was called with SUCCESS status
    ArgumentCaptor<HashMap<String, String>> captor = ArgumentCaptor.forClass(HashMap.class);
    verify(mockPerfController).setFSPTimingInfo(eq(timestamp), captor.capture());
    HashMap<String, String> info = captor.getValue();
    assertEquals(FSPTracer.ResultStatus.SUCCESS.getValue(), info.get("fspStatus"));
  }

  @Test
  public void testIsSnapshotValuable_InvalidXFill() {
    // Create a snapshot with insufficient X fill
    FSPSnapshot snapshot = new FSPSnapshot(containerWidth, containerHeight, 1000000);
    // Only add a small vertical strip that covers little of X axis
    snapshot.fillContentToSnapshot(true, new Rect(containerWidth, 0, 50, containerHeight), 1000000);
    // Directly test the public method
    boolean isValuable = tracer.isSnapshotValuable(snapshot, config);
    assertFalse(isValuable);
  }

  @Test
  public void testIsSnapshotValuable_InvalidYFill() {
    // Create a snapshot with insufficient Y fill
    FSPSnapshot snapshot = new FSPSnapshot(containerWidth, containerHeight, 1000000);
    // Only add a small horizontal strip that covers little of Y axis
    snapshot.fillContentToSnapshot(true, new Rect(0, containerHeight, containerWidth, 50), 1000000);
    // Directly test the public method
    boolean isValuable = tracer.isSnapshotValuable(snapshot, config);
    assertFalse(isValuable);
  }

  @Test
  public void testIsSnapshotValuable_InvalidTotalAreaFill() {
    // Create a snapshot with insufficient total area fill
    FSPSnapshot snapshot = new FSPSnapshot(containerWidth, containerHeight, 1000000);
    // Add scattered small areas that don't reach total area threshold
    snapshot.fillContentToSnapshot(false, new Rect(0, 0, 300, 300), 1000000);
    snapshot.fillContentToSnapshot(true, new Rect(0, 0, 10, 10), 1000000);
    snapshot.fillContentToSnapshot(true, new Rect(100, 100, 10, 10), 1000000);
    // Directly test the public method
    boolean isValuable = tracer.isSnapshotValuable(snapshot, config);
    assertFalse(isValuable);
  }

  @Test
  public void testIsSnapshotValuable_Valid() {
    // Create a valuable snapshot
    FSPSnapshot snapshot = createValuableSnapshot(1000000);
    // Directly test the public method
    boolean isValuable = tracer.isSnapshotValuable(snapshot, config);

    assertTrue(isValuable);
    assertTrue(snapshot.getContentFillPercentageX() >= 30);
    assertTrue(snapshot.getContentFillPercentageY() >= 30);
    assertTrue(snapshot.getContentFillPercentageTotalArea() >= 30);
  }

  @Test
  public void testIsSnapshotStable_StableSnapshots() {
    // Create two stable snapshots
    long timestamp = 1000000;
    FSPSnapshot firstSnapshot = createValuableSnapshot(timestamp);
    FSPSnapshot secondSnapshot =
        createValuableSnapshot((timestamp + (long) ((config.minDiffIntervalMs + 1) * 1000)));
    // Directly test the public method
    boolean isStable = tracer.isSnapshotStable(secondSnapshot, firstSnapshot, config);
    assertTrue(isStable);
  }

  @Test
  public void testIsSnapshotStable_UnstableDueToHighXChangeRate() {
    // Create first snapshot
    FSPSnapshot firstSnapshot = createValuableSnapshot(1000000);
    // Create second snapshot with significant X axis changes
    FSPSnapshot secondSnapshot =
        new FSPSnapshot(containerWidth, containerHeight, 1001000); // 1ms later
    // Add completely different content to create high change rate
    secondSnapshot.fillContentToSnapshot(true, new Rect(100, 0, 300, 800), 1001000);
    // Directly test the public method
    boolean isStable = tracer.isSnapshotStable(secondSnapshot, firstSnapshot, config);

    assertFalse(isStable);
  }

  @Test
  public void testIsSnapshotStable_UnstableDueToHighAreaChangeRate() {
    // Create first snapshot
    FSPSnapshot firstSnapshot = createValuableSnapshot(1000000);
    // Create second snapshot with significant area changes
    FSPSnapshot secondSnapshot =
        new FSPSnapshot(containerWidth, containerHeight, 1001000); // 1ms later
    // Add much more content to create high area change rate
    secondSnapshot.fillContentToSnapshot(
        true, new Rect(0, 0, containerWidth, containerHeight), 1001000);
    // Directly test the public method
    boolean isStable = tracer.isSnapshotStable(secondSnapshot, firstSnapshot, config);

    assertFalse(isStable);
  }

  @Test
  public void testIsSnapshotStable_InvalidTimestamps() {
    // Create snapshots with invalid timestamps
    FSPSnapshot firstSnapshot = createValuableSnapshot(0); // Invalid timestamp
    FSPSnapshot secondSnapshot = createValuableSnapshot(1000000);
    // Directly test the public method
    boolean isStable = tracer.isSnapshotStable(secondSnapshot, firstSnapshot, config);
    assertFalse(isStable);
  }

  @Test
  public void testIsSnapshotStable_NegativeTimeDiff() {
    // Create snapshots with inverted timestamps
    FSPSnapshot firstSnapshot = createValuableSnapshot(2000000);
    FSPSnapshot secondSnapshot = createValuableSnapshot(1000000); // Earlier than first
    // Directly test the public method
    boolean isStable = tracer.isSnapshotStable(secondSnapshot, firstSnapshot, config);
    assertFalse(isStable);
  }

  // Helper methods
  private FSPSnapshot createValuableSnapshot(long timestampUs) {
    FSPSnapshot snapshot = new FSPSnapshot(containerWidth, containerHeight, timestampUs);
    // Add enough content to make it valuable
    snapshot.fillContentToSnapshot(true, new Rect(0, 0, 400, 800), timestampUs);
    snapshot.fillContentToSnapshot(true, new Rect(100, 100, 300, 600), timestampUs);
    snapshot.fillContentToSnapshot(true, new Rect(50, 50, 450, 900), timestampUs);
    return snapshot;
  }

  private void setField(Class clazz, Object target, String fieldName, Object value) {
    try {
      java.lang.reflect.Field field = clazz.getDeclaredField(fieldName);
      field.setAccessible(true);
      field.set(target, value);
    } catch (Exception e) {
      fail("Failed to set minDiffIntervalMs via reflection: " + e.getMessage());
    }
  }

  private Object getField(Class clazz, Object target, String fieldName) {
    try {
      java.lang.reflect.Field field = clazz.getDeclaredField(fieldName);
      field.setAccessible(true);
      return field.get(target);
    } catch (Exception e) {
      fail("Failed to set minDiffIntervalMs via reflection: " + e.getMessage());
    }
    return null;
  }
}
