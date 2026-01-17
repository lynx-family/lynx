// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.Objects;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

/**
 * A {@link Runnable} that wraps another {@link Runnable} to ensure it is executed at most once.
 */
public class RunOnceRunnable implements Runnable {
  public static int STATUS_IDLE = 0;
  public static int STATUS_RUNNING = 1;
  public static int STATUS_EXCEPTION = 2;
  public static int STATUS_FINISHED = 3;

  private final Runnable runnable;
  private final AtomicInteger status = new AtomicInteger(STATUS_IDLE);
  private final AtomicReference<Throwable> exception = new AtomicReference<>(null);
  private final CountDownLatch done = new CountDownLatch(1);

  /**
   * @param runnable the task to be executed only once.
   */
  public RunOnceRunnable(@NonNull Runnable runnable) {
    this.runnable = Objects.requireNonNull(runnable);
    // this.onFinished = onFinished;
  }

  /**
   * Executes the wrapped {@link Runnable} on the first call. Subsequent calls will do nothing.
   */
  @Override
  public void run() {
    // Only can start to run at STATUS_IDLE
    if (status.compareAndSet(STATUS_IDLE, STATUS_RUNNING)) {
      try {
        runnable.run();
        status.set(STATUS_FINISHED);
      } catch (Throwable e) {
        status.set(STATUS_EXCEPTION);
        exception.set(e);
        throw new RuntimeException(e);
      } finally {
        done.countDown();
      }
    }
  }

  /**
   * @return true if status has reached STATUS_FINISHED or STATUS_EXCEPTION.
   */
  public boolean waitForComplete(long timeout, TimeUnit unit) {
    try {
      return done.await(timeout, unit);
    } catch (InterruptedException e) {
      return false;
    }
  }

  /**
   * @return Current status (STATUS_XXX).
   */
  public int getStatus() {
    return status.get();
  }

  /**
   * @return The exception thrown by this runnable.
   */
  public @Nullable Throwable getException() {
    return exception.get();
  }
}
