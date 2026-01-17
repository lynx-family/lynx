// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;

public class RunOnceRunnableTest {
  @Test
  public void testRun_isExecutedOnlyOnce() {
    AtomicInteger executionCount = new AtomicInteger(0);
    Runnable task = executionCount::incrementAndGet;
    RunOnceRunnable runOnce = new RunOnceRunnable(task);

    runOnce.run();
    runOnce.run(); // Second call should do nothing

    assertEquals(1, executionCount.get());
    assertEquals(RunOnceRunnable.STATUS_FINISHED, runOnce.getStatus());
  }

  @Test
  public void testMultiThreadedRun_isExecutedOnlyOnce() throws InterruptedException {
    AtomicInteger executionCount = new AtomicInteger(0);
    Runnable task = () -> {
      try {
        // Simulate some work
        Thread.sleep(50);
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
      executionCount.incrementAndGet();
    };
    final RunOnceRunnable runOnce = new RunOnceRunnable(task);

    final int threadCount = 20;
    final CountDownLatch startLatch = new CountDownLatch(1);
    final CountDownLatch endLatch = new CountDownLatch(threadCount);
    final Thread[] threads = new Thread[threadCount];

    for (int i = 0; i < threadCount; i++) {
      threads[i] = new Thread(() -> {
        try {
          startLatch.await();
          runOnce.run();
        } catch (InterruptedException e) {
          Thread.currentThread().interrupt();
        } finally {
          endLatch.countDown();
        }
      });
      threads[i].start();
    }

    startLatch.countDown(); // Start all threads at once
    endLatch.await(5, TimeUnit.SECONDS);

    assertEquals("The wrapped runnable should be executed exactly once.", 1, executionCount.get());
    assertEquals(RunOnceRunnable.STATUS_FINISHED, runOnce.getStatus());
  }

  @Test
  public void testStatusTransitions() {
    Runnable task = () -> {};
    RunOnceRunnable runOnce = new RunOnceRunnable(task);
    assertEquals(RunOnceRunnable.STATUS_IDLE, runOnce.getStatus());
    runOnce.run();
    assertEquals(RunOnceRunnable.STATUS_FINISHED, runOnce.getStatus());
  }

  @Test
  public void testExceptionHandling() {
    final RuntimeException testException = new RuntimeException("Test Exception");
    Runnable task = () -> {
      throw testException;
    };
    RunOnceRunnable runOnce = new RunOnceRunnable(task);
    Throwable err = null;
    try {
      runOnce.run();
    } catch (Throwable e) {
      err = e;
    }

    assertEquals(RunOnceRunnable.STATUS_EXCEPTION, runOnce.getStatus());
    assertNotNull(runOnce.getException());
    assertNotNull(err);
    assertEquals(testException, err.getCause());
    assertEquals(testException, runOnce.getException());
  }

  @Test
  public void testWaitForComplete_returnsTrueAfterCompletion() throws InterruptedException {
    Runnable task = () -> {
      try {
        Thread.sleep(50);
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    };
    RunOnceRunnable runOnce = new RunOnceRunnable(task);

    new Thread(runOnce).start();

    assertTrue("waitForComplete should return true when the task is finished.",
        runOnce.waitForComplete(1, TimeUnit.SECONDS));
    assertEquals(RunOnceRunnable.STATUS_FINISHED, runOnce.getStatus());
  }

  @Test
  public void testWaitForComplete_timesOutCorrectly() throws InterruptedException {
    Runnable task = () -> {
      try {
        Thread.sleep(500);
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    };
    RunOnceRunnable runOnce = new RunOnceRunnable(task);

    new Thread(runOnce).start();

    assertFalse("waitForComplete should return false on timeout.",
        runOnce.waitForComplete(100, TimeUnit.MILLISECONDS));
    // Even after timeout, the task might still be running
    assertTrue(runOnce.getStatus() == RunOnceRunnable.STATUS_RUNNING
        || runOnce.getStatus() == RunOnceRunnable.STATUS_FINISHED);
  }

  @Test(expected = NullPointerException.class)
  public void testConstructor_withNullRunnable() {
    new RunOnceRunnable(null);
  }
}
