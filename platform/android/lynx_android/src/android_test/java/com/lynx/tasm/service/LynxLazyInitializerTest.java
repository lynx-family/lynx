// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.core.LynxThreadPool;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxLazyInitializerTest {
  private static class TestInitializer extends LynxLazyInitializer {
    private final AtomicInteger doInitializeCallCount = new AtomicInteger(0);
    private final long initializationDelay;
    private final boolean shouldSucceed;

    TestInitializer(long initializationDelay, boolean shouldSucceed) {
      this.initializationDelay = initializationDelay;
      this.shouldSucceed = shouldSucceed;
    }

    @Override
    protected boolean doInitialize() {
      doInitializeCallCount.incrementAndGet();
      if (initializationDelay > 0) {
        try {
          Thread.sleep(initializationDelay);
        } catch (InterruptedException e) {
          Thread.currentThread().interrupt();
        }
      }
      if (!shouldSucceed) {
        throw new RuntimeException("Initialization failed for test.");
      }
      return true;
    }

    int getDoInitializeCallCount() {
      return doInitializeCallCount.get();
    }
  }

  @Test
  public void testInitialize_doInitializeIsCalledOnlyOnce() throws InterruptedException {
    TestInitializer initializer = new TestInitializer(50, true);
    initializer.initialize();
    initializer.initialize(); // Call again

    assertTrue(initializer.ensureInitialized(false));
    assertEquals(1, initializer.getDoInitializeCallCount());
  }

  @Test
  public void testEnsureInitialized_runsSynchronously() {
    TestInitializer initializer = new TestInitializer(0, true);
    assertTrue(initializer.ensureInitialized(true));
    assertEquals(1, initializer.getDoInitializeCallCount());
  }

  @Test
  public void testEnsureInitialized_runsAsynchronouslyAndWait() {
    TestInitializer initializer = new TestInitializer(50, true);
    assertTrue(initializer.ensureInitialized(false));
    assertEquals(1, initializer.getDoInitializeCallCount());
  }

  @Test
  public void testMultiThreadedInitialization_doInitializeIsCalledOnlyOnce()
      throws InterruptedException {
    final TestInitializer initializer = new TestInitializer(100, true);
    final int threadCount = 10;
    final CountDownLatch startLatch = new CountDownLatch(1);
    final CountDownLatch endLatch = new CountDownLatch(threadCount);

    for (int i = 0; i < threadCount; i++) {
      LynxThreadPool.getAsyncServiceExecutor().execute(() -> {
        try {
          startLatch.await();
          initializer.initialize();
        } catch (InterruptedException e) {
          Thread.currentThread().interrupt();
        } finally {
          endLatch.countDown();
        }
      });
    }

    startLatch.countDown();
    endLatch.await(2, TimeUnit.SECONDS);

    assertTrue(initializer.ensureInitialized(false));
    assertEquals(1, initializer.getDoInitializeCallCount());
  }

  @Test
  public void testMultiThreadedEnsureInitialized_doInitializeIsCalledOnlyOnce()
      throws InterruptedException {
    final TestInitializer initializer = new TestInitializer(100, true);
    final int threadCount = 10;
    final CountDownLatch startLatch = new CountDownLatch(1);
    final CountDownLatch endLatch = new CountDownLatch(threadCount);
    final AtomicInteger successCount = new AtomicInteger(0);

    for (int i = 0; i < threadCount; i++) {
      new Thread(() -> {
        try {
          startLatch.await();
          if (initializer.ensureInitialized(false)) {
            successCount.incrementAndGet();
          }
        } catch (InterruptedException e) {
          Thread.currentThread().interrupt();
        } finally {
          endLatch.countDown();
        }
      }).start();
    }

    startLatch.countDown();
    endLatch.await(5, TimeUnit.SECONDS);

    assertEquals(threadCount, successCount.get());
    assertEquals(1, initializer.getDoInitializeCallCount());
  }

  @Test
  public void testInitializationFailure() {
    TestInitializer initializer = new TestInitializer(0, false);
    assertFalse(initializer.ensureInitialized());
    assertEquals(1, initializer.getDoInitializeCallCount());
  }

  @Test
  public void testTimeout() {
    // Timeout is 3 seconds, so we set initialization to take longer.
    TestInitializer initializer = new TestInitializer(3100, true);
    assertFalse(initializer.ensureInitialized(false));
  }
}
