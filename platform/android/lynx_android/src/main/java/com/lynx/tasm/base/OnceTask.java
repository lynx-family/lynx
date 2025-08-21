// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.base;

import androidx.core.util.Consumer;
import java.util.concurrent.Callable;
import java.util.concurrent.FutureTask;
import java.util.concurrent.atomic.AtomicBoolean;

// Using OnceTask can ensure asynchronous tasks execute only once.
public class OnceTask<T> implements Runnable {
  private final static String TAG = "OnceTask";

  private final AtomicBoolean mStarted = new AtomicBoolean(false);

  private final FutureTask<T> mFutureTask;

  private Consumer<Exception> mExceptionHandler = null;

  public OnceTask(final Callable<T> task) {
    mFutureTask = new FutureTask<T>(task);
  }

  public OnceTask(final Callable<T> task, Consumer<Exception> exceptionHandler) {
    mFutureTask = new FutureTask<T>(task);
    mExceptionHandler = exceptionHandler;
  }

  protected void tryRun() {
    if (mStarted.compareAndSet(false, true)) {
      mFutureTask.run();
    }
  }

  @Override
  public void run() {
    tryRun();
  }

  public T get() {
    try {
      return mFutureTask.get();
    } catch (Exception e) {
      LLog.e(TAG, "Get result from OnceTask failed since: " + e.toString());
      if (mExceptionHandler != null) {
        mExceptionHandler.accept(e);
      }
    }
    return null;
  }
}
