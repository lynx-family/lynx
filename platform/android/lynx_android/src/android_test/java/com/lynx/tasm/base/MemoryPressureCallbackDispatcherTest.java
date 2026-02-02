// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import static org.junit.Assert.*;

import androidx.test.filters.SmallTest;
import com.lynx.base.memory.MemoryPressureCallback;
import com.lynx.base.memory.MemoryPressureLevel;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Before;
import org.junit.Test;

/** Test suite for {@link MemoryPressureCallbackDispatcher}. */
public class MemoryPressureCallbackDispatcherTest {
  private MemoryPressureCallbackDispatcher mDispatcher;

  @Before
  public void setUp() {
    mDispatcher = MemoryPressureCallbackDispatcher.getInstance();
  }

  @Test
  @SmallTest
  public void testAddAndNotifyCallback() {
    final AtomicInteger pressureReceived = new AtomicInteger(-1);
    MemoryPressureCallback callback = new MemoryPressureCallback() {
      @Override
      public void onPressure(@MemoryPressureLevel int pressure) {
        pressureReceived.set(pressure);
      }
    };

    mDispatcher.addCallback(callback);
    try {
      mDispatcher.notifyMemoryPressure(MemoryPressureLevel.MODERATE);
      assertEquals(MemoryPressureLevel.MODERATE, pressureReceived.get());

      mDispatcher.notifyMemoryPressure(MemoryPressureLevel.CRITICAL);
      assertEquals(MemoryPressureLevel.CRITICAL, pressureReceived.get());
    } finally {
      mDispatcher.removeCallback(callback);
    }
  }

  @Test
  @SmallTest
  public void testRemoveCallback() {
    final AtomicInteger pressureReceived = new AtomicInteger(-1);
    MemoryPressureCallback callback = new MemoryPressureCallback() {
      @Override
      public void onPressure(@MemoryPressureLevel int pressure) {
        pressureReceived.set(pressure);
      }
    };

    mDispatcher.addCallback(callback);
    mDispatcher.removeCallback(callback);

    pressureReceived.set(-1);
    mDispatcher.notifyMemoryPressure(MemoryPressureLevel.MODERATE);
    assertEquals(-1, pressureReceived.get());
  }

  @Test
  @SmallTest
  public void testMultipleCallbacks() {
    final AtomicInteger count = new AtomicInteger(0);
    MemoryPressureCallback callback1 = p -> count.incrementAndGet();
    MemoryPressureCallback callback2 = p -> count.incrementAndGet();

    mDispatcher.addCallback(callback1);
    mDispatcher.addCallback(callback2);

    try {
      mDispatcher.notifyMemoryPressure(MemoryPressureLevel.MODERATE);
      assertEquals(2, count.get());
    } finally {
      mDispatcher.removeCallback(callback1);
      mDispatcher.removeCallback(callback2);
    }
  }

  @Test
  @SmallTest
  public void testAddIfAbsent() {
    final AtomicInteger count = new AtomicInteger(0);
    MemoryPressureCallback callback = p -> count.incrementAndGet();

    mDispatcher.addCallback(callback);
    mDispatcher.addCallback(callback); // Should not be added again

    try {
      mDispatcher.notifyMemoryPressure(MemoryPressureLevel.MODERATE);
      assertEquals(1, count.get());
    } finally {
      mDispatcher.removeCallback(callback);
    }
  }
}
