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
import java.util.concurrent.CountDownLatch;

public class UIThreadUtils {
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

  public static void runOnUiThreadImmediatelyWithPostAtFront(Runnable runnable) {
    if (isOnUiThread()) {
      runnable.run();
    } else {
      postAtFrontOfQueueOnUiThread(runnable);
    }
  }

  /**
   * Runs the given runnable on the UI thread synchronously.
   *
   * If called from the UI thread, executes immediately.
   * If called from a background thread, posts to the UI thread and blocks until completed.
   */
  public static void runOnUiThreadSync(Runnable runnable) {
    if (isOnUiThread()) {
      runnable.run();
    } else {
      final CountDownLatch latch = new CountDownLatch(1);
      postAtFrontOfQueueOnUiThread(new Runnable() {
        @Override
        public void run() {
          runnable.run();
          latch.countDown();
        }
      });
      try {
        latch.await();
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
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
