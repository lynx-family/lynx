// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.tracing;

import android.os.Looper;
import android.util.Printer;
import androidx.annotation.NonNull;
import com.lynx.devtool.framecapture.FrameTraceUtil;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.TraceEvent;
import java.lang.reflect.Field;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;

public class FPSTrace {
  private long mStartTimestamp;
  private ExecutorService mExecutorService;
  private static final int CALLBACK_INPUT = 0;
  private Runnable mCallback;
  private boolean isVsyncFrame = false;
  private LooperPrinter mPrinter;
  private long mNativeFPSTrace = 0;

  private FPSTrace() {
    mNativeFPSTrace = nativeCreateFPSTrace();
    mCallback = new Runnable() {
      @Override
      public void run() {
        isVsyncFrame = true;
      }
    };
  }

  private static class FPSTraceLoader {
    private static final FPSTrace INSTANCE = new FPSTrace();
  }

  public static FPSTrace getInstance() {
    return FPSTraceLoader.INSTANCE;
  }

  private void addFPSCallback() {
    if (TraceEvent.categoryEnabled(TraceEvent.CATEGORY_FPS)) {
      /**
       * add callback to mCallbackQueues[CALLBACK_INPUT] head,
       * when vsync coming, mCallback will run first
       */
      FrameTraceUtil.addFrameCallback(mCallback);
    }
  }

  @CalledByNative
  public void startFPSTrace() {
    if (TraceEvent.categoryEnabled(TraceEvent.CATEGORY_FPS)) {
      FrameTraceService.getInstance().initializeService();
      mExecutorService = Executors.newCachedThreadPool(new ThreadFactory() {
        @Override
        public Thread newThread(@NonNull Runnable r) {
          return new Thread(r, "FPSTrace");
        }
      });
      try {
        Looper looper = Looper.getMainLooper();
        Field logging = Looper.class.getDeclaredField("mLogging");
        logging.setAccessible(true);
        Printer origin = (Printer) logging.get(looper);
        mPrinter = new LooperPrinter(origin);
        looper.setMessageLogging(mPrinter);
      } catch (Throwable e) {
        e.printStackTrace();
      }
      addFPSCallback();
    }
  }

  @CalledByNative
  public void stopFPSTrace() {
    if (mPrinter != null) {
      Looper.getMainLooper().setMessageLogging(mPrinter.getOrigin());
      mPrinter = null;
    }
    if (mExecutorService != null) {
      mExecutorService.shutdown();
      mExecutorService = null;
    }
  }

  private void record(final long startTimestamp, final long endTimestamp) {
    if (endTimestamp - startTimestamp >= FrameTraceUtil.getFrameIntervalNanos()) {
      if (mExecutorService != null) {
        mExecutorService.submit(new Runnable() {
          @Override
          public void run() {
            FrameTraceService.getInstance().FPSTrace(startTimestamp, endTimestamp);
          }
        });
      }
    }
  }

  private void dispatchBegin() {
    isVsyncFrame = false;
    mStartTimestamp = FrameTraceUtil.getSystemBootTimeNs();
  }

  private void dispatchEnd() {
    if (isVsyncFrame) {
      isVsyncFrame = false;
      addFPSCallback();
      final long endTimestamp = FrameTraceUtil.getSystemBootTimeNs();
      record(mStartTimestamp, endTimestamp);
    }
  }

  public long getNativeFPSTrace() {
    return mNativeFPSTrace;
  }

  private class LooperPrinter implements Printer {
    private Printer mOrigin;

    public LooperPrinter(Printer origin) {
      mOrigin = origin;
    }

    public Printer getOrigin() {
      return mOrigin;
    }

    @Override
    public void println(String x) {
      if (mOrigin != null) {
        mOrigin.println(x);
      }
      if (x == null) {
        return;
      }
      if (x.charAt(0) == '>') {
        dispatchBegin();
      } else if (x.charAt(0) == '<') {
        dispatchEnd();
      }
    }
  }

  private native long nativeCreateFPSTrace();
}
