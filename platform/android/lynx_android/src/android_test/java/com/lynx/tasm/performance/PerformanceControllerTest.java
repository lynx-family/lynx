// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntryConverter;
import java.lang.reflect.Field;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class PerformanceControllerTest {
  private PerformanceController mPerformanceController;

  private static Field mNativeActorPtrField = null;

  @Mock private IPerformanceObserver mMockObserver;

  @Mock private ReadableMap mMockEntryMap;

  @Mock private PerformanceEntry mMockEntry;

  @Before
  public void setUp() throws NoSuchFieldException {
    if (mNativeActorPtrField == null) {
      mNativeActorPtrField =
          PerformanceController.class.getDeclaredField("mNativePerformanceActorPtr");
      mNativeActorPtrField.setAccessible(true);
    }
    mPerformanceController = new PerformanceController();
    mPerformanceController.init();
  }

  @Test
  public void testConstructor() throws IllegalAccessException {
    assertNotNull(mPerformanceController);
    assertTrue(mNativeActorPtrField.getLong(mPerformanceController) != 0);
  }

  @Test
  public void testDestroy() throws IllegalAccessException {
    mPerformanceController.destroy();
    assertEquals(0L, mNativeActorPtrField.getLong(mPerformanceController));
  }

  @Test
  public void testAllocateMemory_WhenNativePtrIsZero() throws IllegalAccessException {
    // Test when native pointer is 0 (should do nothing)
    mNativeActorPtrField.setLong(mPerformanceController, 0L);
    mPerformanceController.allocateMemory("test", 1.0f, "key", "value");
    mPerformanceController.deallocateMemory("test", 1.0f, "key", "value");
    mPerformanceController.updateMemoryUsage("test", 1.0f, "key", "value");
    // Test passes if no exception is thrown
  }

  @Test
  public void testAllocateMemory_WhenNativePtrIsValid() {
    mPerformanceController.allocateMemory("test", 1.0f, "key", "value");
    // Can only verify no exception is thrown since it's a native method
  }

  @Test
  public void testDeallocateMemory_WhenNativePtrIsValid() {
    mPerformanceController.deallocateMemory("test", 1.0f, "key", "value");
    // Can only verify no exception is thrown since it's a native method
  }

  @Test
  public void testUpdateMemoryUsage_WhenNativePtrIsValid() {
    mPerformanceController.updateMemoryUsage("test", 1.0f, "key", "value");
    // Can only verify no exception is thrown since it's a native method
  }

  @Test
  public void testGetNativePtr() {
    long ptr = mPerformanceController.getNativePtr();
    assertEquals(12345L, ptr);
  }

  @Test
  public void testOnPerformanceEvent_WithObserver() {
    mPerformanceController.setPerformanceObserver(mMockObserver);
    when(PerformanceEntryConverter.makePerformanceEntry(mMockEntryMap)).thenReturn(mMockEntry);

    mPerformanceController.onPerformanceEvent(mMockEntryMap);

    verify(mMockObserver).onPerformanceEvent(mMockEntry);
  }

  @Test
  public void testOnPerformanceEvent_WithoutObserver() {
    when(PerformanceEntryConverter.makePerformanceEntry(mMockEntryMap)).thenReturn(mMockEntry);

    mPerformanceController.onPerformanceEvent(mMockEntryMap);

    // Verify no interaction with observer when none is set
    verifyNoInteractions(mMockObserver);
  }
}
