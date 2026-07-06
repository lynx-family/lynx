// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.tasm.utils;

import android.os.Handler;
import android.os.Looper;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.Assertions;
import com.lynx.tasm.base.LLog;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;

public class UIThreadUtils {
  private static final String TAG = "UIThreadUtils";

  @Nullable private static volatile Handler sMainHandler;

  private static Handler getUiThreadHandler() {
    // Double null-check to avoid unnecessary lock
    if (sMainHandler == null) {
      synchronized (UIThreadUtils.class) {
        if (sMainHandler == null) {
          sMainHandler = new Handler(Looper.getMainLooper());
        }
      }
    }
    return sMainHandler;
  }

  public static boolean isOnUiThread() {
    return Looper.getMainLooper().getThread() == Thread.currentThread();
  }

  public static void assertOnUiThread() {
    Assertions.assertCondition(isOnUiThread(), "Expected to run on UI thread!");
  }

  public static void assertNotOnUiThread() {
    Assertions.assertCondition(!isOnUiThread(), "Expected not to run on UI thread!");
  }

  public static void runOnUiThread(Runnable runnable) {
    getUiThreadHandler().post(runnable);
  }

  public static void postAtFrontOfQueueOnUiThread(Runnable runnable) {
    getUiThreadHandler().postAtFrontOfQueue(runnable);
  }

  public static void runOnUiThreadImmediately(Runnable runnable) {
    if (isOnUiThread()) {
      runnable.run();
    } else {
      runOnUiThread(runnable);
    }
  }

  /**
   * Runs the given task on the UI thread and waits for posted execution to finish.
   *
   * <p>Returns {@code true} only when the task has completed before this method returns. Returns
   * {@code false} if the waiting thread is interrupted or if the posted task throws. When called
   * from the UI thread, the task runs inline and exceptions are propagated. Avoid calling this
   * while holding locks that the UI task may need.
   */
  public static boolean runOnUiThreadSync(Runnable runnable) {
    if (isOnUiThread()) {
      runnable.run();
      return true;
    }
    FutureTask<Void> task = new FutureTask<>(runnable, null);
    runOnUiThread(task);
    try {
      task.get();
      return true;
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
      LLog.e(TAG, "Interrupted while waiting for UI thread task: " + e);
      return false;
    } catch (ExecutionException e) {
      LLog.e(TAG, "Exception while running UI thread task: " + e.getCause());
      return false;
    }
  }

  public static void runOnUiThreadImmediatelyWithPostAtFront(Runnable runnable) {
    if (isOnUiThread()) {
      runnable.run();
    } else {
      postAtFrontOfQueueOnUiThread(runnable);
    }
  }

  public static void runOnUiThread(Runnable runnable, long delayMs) {
    getUiThreadHandler().postDelayed(runnable, delayMs);
  }

  public static void runOnUiThreadAtTime(Runnable runnable, Object token, long uptimeMillis) {
    getUiThreadHandler().postAtTime(runnable, token, uptimeMillis);
  }

  public static void removeCallbacks(Runnable r, Object token) {
    getUiThreadHandler().removeCallbacks(r, token);
  }
}
