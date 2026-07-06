// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import androidx.test.filters.SmallTest;
import androidx.test.platform.app.InstrumentationRegistry;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;

public class UIThreadUtilsTest {
  @Test
  @SmallTest
  public void runOnUiThreadSyncRunsInlineOnUiThread() {
    AtomicBoolean completed = new AtomicBoolean(false);
    AtomicBoolean result = new AtomicBoolean(false);

    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      result.set(UIThreadUtils.runOnUiThreadSync(() -> {
        assertTrue(UIThreadUtils.isOnUiThread());
        completed.set(true);
      }));
    });

    assertTrue(result.get());
    assertTrue(completed.get());
  }

  @Test
  @SmallTest
  public void runOnUiThreadSyncWaitsForTaskCompletionFromWorkerThread()
      throws InterruptedException {
    AtomicBoolean completed = new AtomicBoolean(false);
    AtomicBoolean completedBeforeReturn = new AtomicBoolean(false);
    AtomicBoolean result = new AtomicBoolean(false);
    AtomicBoolean ranOnUiThread = new AtomicBoolean(false);

    Thread worker = new Thread(() -> {
      result.set(UIThreadUtils.runOnUiThreadSync(() -> {
        ranOnUiThread.set(UIThreadUtils.isOnUiThread());
        completed.set(true);
      }));
      completedBeforeReturn.set(completed.get());
    });

    worker.start();
    worker.join(3000);

    assertFalse(worker.isAlive());
    assertTrue(result.get());
    assertTrue(ranOnUiThread.get());
    assertTrue(completedBeforeReturn.get());
  }

  @Test
  @SmallTest
  public void runOnUiThreadSyncReturnsFalseWhenWaitingThreadIsInterrupted()
      throws InterruptedException {
    AtomicBoolean interruptPreserved = new AtomicBoolean(false);
    AtomicBoolean result = new AtomicBoolean(true);

    Thread worker = new Thread(() -> {
      Thread.currentThread().interrupt();
      result.set(UIThreadUtils.runOnUiThreadSync(() -> {}));
      interruptPreserved.set(Thread.currentThread().isInterrupted());
    });

    worker.start();
    worker.join(3000);

    assertFalse(worker.isAlive());
    assertFalse(result.get());
    assertTrue(interruptPreserved.get());
  }

  @Test
  @SmallTest
  public void runOnUiThreadSyncReturnsFalseWhenPostedTaskThrows() throws InterruptedException {
    AtomicBoolean result = new AtomicBoolean(true);
    AtomicReference<Throwable> workerError = new AtomicReference<>();

    Thread worker = new Thread(() -> {
      try {
        result.set(UIThreadUtils.runOnUiThreadSync(
            () -> { throw new RuntimeException("test exception"); }));
      } catch (Throwable throwable) {
        workerError.set(throwable);
      }
    });

    worker.start();
    worker.join(3000);

    assertFalse(worker.isAlive());
    assertFalse(result.get());
    assertTrue(workerError.get() == null);
  }
}
