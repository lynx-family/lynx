// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import com.lynx.tasm.base.LLog;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.utils.RunOnceRunnable;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * A helper base class for performing expensive initialization lazily.
 * <p>
 * Subclasses must implement {@link #doInitialize()} to perform the actual initialization logic.
 * This logic is guaranteed to be executed at most once.
 * <p>
 * Initialization can be triggered asynchronously via {@link #initialize()} or performed
 * synchronously by calling {@link #ensureInitialized(boolean)} with {@code true}.
 */
public abstract class LynxLazyInitializer {
  private static final String TAG = "LynxLazyInitializer";

  /**
   * Timeout for waiting for asynchronous initialization to complete, in seconds.
   */
  private static final long TIMEOUT = 3;

  private final RunOnceRunnable runnable = new RunOnceRunnable(this::runCatching);
  private final AtomicBoolean initialized = new AtomicBoolean(false);

  /**
   * Performs the actual initialization. This method will be invoked at most once.
   * <p>
   * Subclasses must implement this method to contain the initialization logic.
   * It will be executed on a background thread if {@link #initialize()} is called, or on the
   * calling thread if {@link #ensureInitialized(boolean)} is called with {@code true}.
   *
   * @return {@code true} if initialization was successful, {@code false} otherwise.
   */
  protected abstract boolean doInitialize();

  private void runCatching() {
    try {
      initialized.set(doInitialize());
    } catch (Exception e) {
      // pass
      LLog.e(TAG, "initialize failed: " + e);
    }
  }

  /**
   * Triggers the initialization asynchronously on a background thread if it has not already
   * started. This method returns immediately and does not block.
   */
  public void initialize() {
    if (runnable.getStatus() == RunOnceRunnable.STATUS_IDLE) {
      LynxThreadPool.getAsyncServiceExecutor().execute(runnable);
    }
  }

  /**
   * Ensures that the initialization is complete, with an option for synchronous execution.
   *
   * @param runImmediately If {@code true}, and initialization has not yet run, this method
   *                       will try to execute the initialization logic synchronously on the
   *                       current thread.
   *                       <p>
   *                       If {@code false}, this method will trigger asynchronous initialization
   *                       (if not already started) and block the current thread until it
   *                       completes or the {@param #TIMEOUT} is reached.
   * @return {@code true} if initialization is considered complete, {@code false} otherwise.
   *         Returning false means this task throw an exception or timeout.
   */
  public boolean ensureInitialized(boolean runImmediately) {
    if (runnable.getStatus() == RunOnceRunnable.STATUS_FINISHED) {
      // fast path
      return true;
    }
    if (!runImmediately) {
      initialize();
    } else {
      // If a background run has already started, RunOnceRunnable will ignore this run,
      //  so just call it.
      runnable.run();
    }
    if (!runnable.waitForComplete(TIMEOUT, TimeUnit.SECONDS)) {
      LLog.e(TAG, "ensureInitialized timeout");
      return false;
    }
    return initialized.get();
  }

  /**
   * Ensures the class is initialized by running the initialization logic synchronously on the
   * calling thread if it has not already run.
   * <p>
   * This is a convenience method for {@code ensureInitialized(true)}.
   *
   * @return {@code true} if initialization is considered complete, {@code false} otherwise.
   *         Returning false means this task throw an exception or timeout.
   */
  public boolean ensureInitialized() {
    return ensureInitialized(true);
  }
}
