// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.core;

import android.util.Log;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.LynxEnv;
import java.util.concurrent.Callable;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.FutureTask;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicInteger;

public final class LynxThreadPool {
  private static final String TAG = "lynx_LynxThreadPool";

  private enum ConcurrentTaskType { HIGH_PRIORITY }

  private LynxThreadPool() {}

  /**
   * brief io executor, not recommended to run the time-consuming tasks
   * <p>
   * thread priority: Thread.NORM_PRIORITY - 2
   */

  private static volatile Executor sBriefIOExecutor;

  private static volatile Executor sCardServiceExecutor;

  private static volatile Executor sImageRequestExecutor;

  private static volatile Executor sSvgRenderExecutor;

  private static volatile Executor sAsyncLepusBridgeExecutor;

  @NonNull
  public static Executor getBriefIOExecutor() {
    if (sBriefIOExecutor == null) {
      synchronized (LynxThreadPool.class) {
        if (null == sBriefIOExecutor) {
          sBriefIOExecutor = getExecutor("lynx-brief-io-thread", Thread.NORM_PRIORITY - 2, 2);
        }
      }
    }
    return sBriefIOExecutor;
  }

  @NonNull
  public static Executor getAsyncServiceExecutor() {
    if (sCardServiceExecutor == null) {
      synchronized (LynxThreadPool.class) {
        if (null == sCardServiceExecutor) {
          sCardServiceExecutor = getExecutor("lynx-card-service-thread", Thread.MAX_PRIORITY, 2);
        }
      }
    }
    return sCardServiceExecutor;
  }

  @NonNull
  public static Future<Runnable> postUIOperationTask(Callable<Runnable> callable) {
    final FutureTask<Runnable> future = new FutureTask<>(callable);
    postTask(new Runnable() {
      @Override
      public void run() {
        future.run();
      }
    }, ConcurrentTaskType.HIGH_PRIORITY);
    return future;
  }

  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public static void postUIOperationTask(Runnable callable) {
    postTask(callable, ConcurrentTaskType.HIGH_PRIORITY);
  }

  public static Executor getImageRequestExecutor() {
    if (sImageRequestExecutor == null) {
      synchronized (LynxThreadPool.class) {
        if (null == sImageRequestExecutor) {
          sImageRequestExecutor = getExecutor("lynx-image-request-thread", Thread.MAX_PRIORITY, 1);
        }
      }
    }
    return sImageRequestExecutor;
  }

  public static Executor getSvgRenderExecutor() {
    if (sSvgRenderExecutor == null) {
      synchronized (LynxThreadPool.class) {
        if (null == sSvgRenderExecutor) {
          sSvgRenderExecutor = getExecutor("lynx-svg-thread", Thread.NORM_PRIORITY - 2, 1);
        }
      }
    }
    return sSvgRenderExecutor;
  }

  public static Executor getAsyncLepusBridgeExecutor() {
    if (sAsyncLepusBridgeExecutor == null) {
      synchronized (LynxThreadPool.class) {
        if (null == sAsyncLepusBridgeExecutor) {
          sAsyncLepusBridgeExecutor =
              getExecutor("lepus-bridge-async-thread", Thread.NORM_PRIORITY - 2, 1);
        }
      }
    }
    return sAsyncLepusBridgeExecutor;
  }

  @NonNull
  private static boolean postTask(Runnable task, ConcurrentTaskType task_type) {
    return LynxEnv.runJavaTaskOnConcurrentLoop(task, task_type.ordinal());
  }

  private static Executor getExecutor(final String name, final int priority, int threadNum) {
    Executor executor = null;
    try {
      // TODO(huangweiwu): Maybe we need cache threads instead of fixed threads sometimes
      executor = Executors.newFixedThreadPool(threadNum, new ThreadFactory() {
        private final AtomicInteger threadNumber = new AtomicInteger(1);
        @Override
        public Thread newThread(Runnable r) {
          Thread thread = new Thread(r, name + "-" + threadNumber.getAndIncrement());
          if (thread.isDaemon()) {
            thread.setDaemon(false);
          }
          thread.setPriority(priority);
          thread.setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
              Log.e(TAG, e.toString());
            }
          });
          return thread;
        }
      });
    } catch (Throwable e) {
      Log.e(TAG, e.toString());
      executor = new Executor() {
        @Override
        public void execute(Runnable command) {
          // thread pool create failed.so just do nothing when use pool.
        }
      };
    }
    return executor;
  }
}
