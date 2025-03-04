// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import android.os.Looper;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.core.LynxThreadPool;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Helper base class for lazy initializing
 */
public abstract class LynxLazyInitializer {
  private static final String TAG = "LynxLazyInitializer";

  /**
   * timeout 5s
   */
  private static final long TIMEOUT = 5;

  private final AtomicBoolean mIsInitializeStarted = new AtomicBoolean(false);
  protected final AtomicBoolean mIsInitialized = new AtomicBoolean(false);
  private final CountDownLatch countDownLatch = new CountDownLatch(1);

  /**
   * Initialize the class, need to be implemented by subclass.
   * This method will be invoked by `initialize()` in a background thread. Do not call this method
   * directly, use `initialize()` instead.
   * @return true if initialized, false if failed
   */
  protected abstract boolean doInitialize();

  /**
   * Initialize the class, will call `doInitialize` in a background thread
   * It should be called early in the lifecycle of the class.
   */
  public void initialize() {
    if (mIsInitializeStarted.getAndSet(true)) {
      return;
    }

    final Runnable initTask = new Runnable() {
      @Override
      public void run() {
        boolean res = doInitialize();
        mIsInitialized.set(res);
        countDownLatch.countDown();
      }
    };

    if (Looper.myLooper() == Looper.getMainLooper()) {
      LynxThreadPool.getBriefIOExecutor().execute(initTask);
    } else {
      initTask.run();
    }
  }

  /**
   * Ensure the class is initialized, will call `initialize` if not initialized with a timeout of
   * 5s. It should be called before using the methods of the class.
   * @return true if initialized, false if timeout
   */
  public boolean ensureInitialized() {
    if (!mIsInitialized.get()) {
      initialize();
      try {
        if (!countDownLatch.await(TIMEOUT, TimeUnit.SECONDS)) {
          LLog.e(TAG, "ensureInitialized timeout");
        }
      } catch (InterruptedException e) {
        LLog.e(TAG, "ensureInitialized failed: " + e);
      }
    }
    return mIsInitialized.get();
  }
}
