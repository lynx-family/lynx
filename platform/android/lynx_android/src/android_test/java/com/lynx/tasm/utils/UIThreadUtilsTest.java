// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.os.Looper;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;

public class UIThreadUtilsTest {
  @Test
  public void testRunOnUiThreadSync_fromMainThread_executesImmediately() {
    AtomicBoolean executed = new AtomicBoolean(false);
    AtomicBoolean onMainThread = new AtomicBoolean(false);

    UIThreadUtils.runOnUiThreadSync(() -> {
      executed.set(true);
      onMainThread.set(Looper.getMainLooper().getThread() == Thread.currentThread());
    });

    assertTrue("Task should be executed", executed.get());
    assertTrue("Task should run on main thread", onMainThread.get());
  }

  @Test
  public void testRunOnUiThreadSync_fromBackgroundThread_blocksUntilComplete()
      throws InterruptedException {
    AtomicBoolean executed = new AtomicBoolean(false);
    AtomicBoolean onMainThread = new AtomicBoolean(false);
    CountDownLatch startedLatch = new CountDownLatch(1);
    CountDownLatch completedLatch = new CountDownLatch(1);

    Thread backgroundThread = new Thread(() -> {
      startedLatch.countDown();
      UIThreadUtils.runOnUiThreadSync(() -> {
        executed.set(true);
        onMainThread.set(Looper.getMainLooper().getThread() == Thread.currentThread());
      });
      completedLatch.countDown();
    });

    backgroundThread.start();
    assertTrue("Background thread should start", startedLatch.await(1, TimeUnit.SECONDS));
    assertTrue("Background thread should complete after sync execution",
        completedLatch.await(2, TimeUnit.SECONDS));

    assertTrue("Task should be executed", executed.get());
    assertTrue("Task should run on main thread", onMainThread.get());
  }

  @Test
  public void testRunOnUiThreadSync_fromBackgroundThread_executesBeforeReturn()
      throws InterruptedException {
    AtomicReference<String> executionOrder = new AtomicReference<>("");
    CountDownLatch startedLatch = new CountDownLatch(1);

    Thread backgroundThread = new Thread(() -> {
      startedLatch.countDown();
      UIThreadUtils.runOnUiThreadSync(() -> { executionOrder.updateAndGet(s -> s + "task"); });
      executionOrder.updateAndGet(s -> s + "after");
    });

    backgroundThread.start();
    assertTrue("Background thread should start", startedLatch.await(1, TimeUnit.SECONDS));
    backgroundThread.join(2000);

    assertFalse("Background thread should not be alive", backgroundThread.isAlive());
    assertEquals(
        "Task should execute before the method returns", "taskafter", executionOrder.get());
  }
}
