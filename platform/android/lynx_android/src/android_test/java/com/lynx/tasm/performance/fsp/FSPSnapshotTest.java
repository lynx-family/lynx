// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.fsp;

import static org.junit.Assert.*;

import android.graphics.Rect;
import java.util.BitSet;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class FSPSnapshotTest {
  @Test
  public void testDefaultConstructor() {
    FSPSnapshot snapshot = new FSPSnapshot();
    assertNotNull(snapshot);
    assertEquals(0, snapshot.getContainerWidth());
    assertEquals(0, snapshot.getContainerHeight());
    assertEquals(0, snapshot.getTotalContentArea());
    assertEquals(0, snapshot.getTotalPresentedContentArea());
    assertEquals(0, snapshot.getLastChangeTimestampUs());
  }

  @Test
  public void testParameterizedConstructor() {
    int containerWidth = 100;
    int containerHeight = 200;
    long timestamp = 123456789;
    FSPSnapshot snapshot = new FSPSnapshot(containerWidth, containerHeight, timestamp);

    assertEquals(containerWidth, snapshot.getContainerWidth());
    assertEquals(containerHeight, snapshot.getContainerHeight());
    assertEquals(timestamp, snapshot.getLastChangeTimestampUs());
  }

  @Test
  public void testSettersAndGetters() {
    // Test container size
    int containerWidth = 300;
    int containerHeight = 400;
    long timestamp = 123456789;
    FSPSnapshot snapshot = new FSPSnapshot(containerWidth, containerHeight, timestamp);
    assertEquals(containerWidth, snapshot.getContainerWidth());
    assertEquals(containerHeight, snapshot.getContainerHeight());

    // Test other properties
    snapshot.setTotalContentArea(1000);
    assertEquals(1000, snapshot.getTotalContentArea());

    snapshot.setTotalPresentedContentArea(500);
    assertEquals(500, snapshot.getTotalPresentedContentArea());

    snapshot.setLastChangeTimestampUs(987654321);
    assertEquals(987654321, snapshot.getLastChangeTimestampUs());

    snapshot.setContentFillPercentageX(25);
    assertEquals(25, snapshot.getContentFillPercentageX());

    snapshot.setContentFillPercentageY(75);
    assertEquals(75, snapshot.getContentFillPercentageY());

    snapshot.setContentFillPercentageTotalArea(50);
    assertEquals(50, snapshot.getContentFillPercentageTotalArea());
  }

  @Test
  public void testFillContentToSnapshot_normalCase() {
    // Arrange
    FSPSnapshot snapshot = new FSPSnapshot(100, 100, 0);
    Rect rect = new Rect(20, 30, 60, 80);
    boolean isPresented = true;
    long timestamp = 1000;

    // Act
    snapshot.fillContentToSnapshot(isPresented, rect, timestamp);

    // Assert
    assertEquals((rect.bottom - rect.top) * (rect.right - rect.left),
        snapshot.getTotalContentArea()); // (60-20)*(80-30) = 40*50=2000
    assertEquals((rect.bottom - rect.top) * (rect.right - rect.left),
        snapshot.getTotalPresentedContentArea());
    assertEquals(timestamp, snapshot.getLastChangeTimestampUs());

    // Verify projections
    BitSet xProjections = snapshot.getXProjections();
    BitSet yProjections = snapshot.getYProjections();
    BitSet xTotalProjections = snapshot.getXTotalContentProjections();
    BitSet yTotalProjections = snapshot.getYTotalContentProjections();

    // Calculate expected projection ranges
    int minProjX = (int) ((long) 20 * 256 / 100); // ~51
    int maxProjX = (int) ((long) 60 * 256 / 100); // ~153
    int minProjY = (int) ((long) 30 * 512 / 100); // ~153
    int maxProjY = (int) ((long) 80 * 512 / 100); // ~409

    // Verify that expected bits are set
    for (int i = minProjX; i <= maxProjX; i++) {
      assertTrue(xProjections.get(i));
      assertTrue(xTotalProjections.get(i));
    }

    for (int i = minProjY; i <= maxProjY; i++) {
      assertTrue(yProjections.get(i));
      assertTrue(yTotalProjections.get(i));
    }
  }

  @Test
  public void testFillContentToSnapshot_notPresented() {
    FSPSnapshot snapshot = new FSPSnapshot(100, 100, 0);
    Rect rect = new Rect(20, 30, 60, 80);
    boolean isPresented = false;
    long timestamp = 1000;

    snapshot.fillContentToSnapshot(isPresented, rect, timestamp);

    assertEquals(
        (rect.bottom - rect.top) * (rect.right - rect.left), snapshot.getTotalContentArea());
    assertEquals(0, snapshot.getTotalPresentedContentArea()); // Should not be added
    assertEquals(timestamp, snapshot.getLastChangeTimestampUs());

    // Verify that xProjections and yProjections are not set
    BitSet xProjections = snapshot.getXProjections();
    BitSet yProjections = snapshot.getYProjections();

    // Calculate expected projection ranges
    int minProjX = (int) ((long) 20 * 256 / 100);
    int maxProjX = (int) ((long) 60 * 256 / 100);
    int minProjY = (int) ((long) 30 * 512 / 100);
    int maxProjY = (int) ((long) 80 * 512 / 100);

    // Verify that xTotalProjections and yTotalProjections are set, but not the presented ones
    BitSet xTotalProjections = snapshot.getXTotalContentProjections();
    BitSet yTotalProjections = snapshot.getYTotalContentProjections();

    for (int i = minProjX; i <= maxProjX; i++) {
      assertFalse(xProjections.get(i));
      assertTrue(xTotalProjections.get(i));
    }

    for (int i = minProjY; i <= maxProjY; i++) {
      assertFalse(yProjections.get(i));
      assertTrue(yTotalProjections.get(i));
    }
  }

  @Test
  public void testFillContentToSnapshot_invalidRect() {
    FSPSnapshot snapshot = new FSPSnapshot(100, 100, 0);

    // Test with empty rect
    Rect emptyRect = new Rect();
    snapshot.fillContentToSnapshot(true, emptyRect, 1000);

    assertEquals(0, snapshot.getTotalContentArea());
    assertEquals(0, snapshot.getTotalPresentedContentArea());

    // Test with null rect
    snapshot.fillContentToSnapshot(true, null, 2000);

    assertEquals(0, snapshot.getTotalContentArea());
    assertEquals(0, snapshot.getTotalPresentedContentArea());
  }

  @Test
  public void testFillContentToSnapshot_zeroSizeContainer() {
    // Test with zero width container
    FSPSnapshot snapshot1 = new FSPSnapshot(0, 100, 0);
    snapshot1.fillContentToSnapshot(true, new Rect(10, 10, 50, 50), 1000);
    assertEquals(0, snapshot1.getTotalContentArea());

    // Test with zero height container
    FSPSnapshot snapshot2 = new FSPSnapshot(100, 0, 0);
    snapshot2.fillContentToSnapshot(true, new Rect(10, 10, 50, 50), 1000);
    assertEquals(0, snapshot2.getTotalContentArea());
  }

  @Test
  public void testFillContentToSnapshot_outsideBounds() {
    FSPSnapshot snapshot = new FSPSnapshot(100, 100, 0);

    // Test with rect completely outside bounds
    Rect outsideRect = new Rect(150, 150, 200, 200);
    snapshot.fillContentToSnapshot(true, outsideRect, 1000);

    assertEquals(0, snapshot.getTotalContentArea());
    assertEquals(0, snapshot.getTotalPresentedContentArea());
  }

  @Test
  public void testFillContentToSnapshot_partialOverlap() {
    FSPSnapshot snapshot = new FSPSnapshot(100, 100, 0);

    // Test with rect partially outside bounds
    Rect partialRect = new Rect(80, 80, 120, 120);
    snapshot.fillContentToSnapshot(true, partialRect, 1000);

    // Only the overlapping area should be counted
    int expectedArea = (100 - 80) * (100 - 80);
    assertEquals(expectedArea, snapshot.getTotalContentArea());
    assertEquals(expectedArea, snapshot.getTotalPresentedContentArea());
  }

  @Test
  public void testFillContentToSnapshot_timestampUpdate() {
    FSPSnapshot snapshot = new FSPSnapshot(100, 100, 0);

    // First update with timestamp 1000
    snapshot.fillContentToSnapshot(true, new Rect(10, 10, 50, 50), 1000);
    assertEquals(1000, snapshot.getLastChangeTimestampUs());

    // Update with older timestamp should not change
    snapshot.fillContentToSnapshot(true, new Rect(50, 50, 90, 90), 500);
    assertEquals(1000, snapshot.getLastChangeTimestampUs());

    // Update with newer timestamp should change
    snapshot.fillContentToSnapshot(true, new Rect(20, 20, 60, 60), 2000);
    assertEquals(2000, snapshot.getLastChangeTimestampUs());
  }

  @Test
  public void testFillContentToSnapshot_multipleCalls() {
    FSPSnapshot snapshot = new FSPSnapshot(100, 100, 0);

    // Add first content
    snapshot.fillContentToSnapshot(true, new Rect(10, 10, 30, 30), 1000);
    int area1 = (30 - 10) * (30 - 10);
    assertEquals(area1, snapshot.getTotalContentArea());
    assertEquals(area1, snapshot.getTotalPresentedContentArea());

    // Add second content
    snapshot.fillContentToSnapshot(true, new Rect(20, 20, 40, 40), 2000);
    int area2 = (40 - 20) * (40 - 20);
    assertEquals(area1 + area2, snapshot.getTotalContentArea());
    assertEquals(area1 + area2, snapshot.getTotalPresentedContentArea());
  }
}
