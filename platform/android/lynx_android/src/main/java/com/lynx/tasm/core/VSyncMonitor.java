// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core;

import android.view.Choreographer;
import android.view.WindowManager;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.utils.CallStackUtil;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;

public class VSyncMonitor {
  public final static long DEFAULT_FRAME_TIME_NS = 1000000000 / 60;
  private static WeakReference<WindowManager> mWindowManager;
  private static Choreographer sUIThreadChoreographer = null;
  public static void setCurrentWindowManager(WindowManager vm) {
    mWindowManager = new WeakReference<>(vm);
  }

  public static void initUIThreadChoreographer() {
    if (sUIThreadChoreographer != null) {
      return;
    }

    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        try {
          sUIThreadChoreographer = Choreographer.getInstance();
        } catch (RuntimeException e) {
          LLog.e("VSyncMonitor",
              "initUIThreadChoreographer failed: " + CallStackUtil.getStackTraceStringTrimmed(e));
        }
      }
    });
  }

  @CalledByNative
  public static void request(final long nativePtr) {
    Choreographer.getInstance().postFrameCallback(new Choreographer.FrameCallback() {
      @Override
      public void doFrame(long frameTimeNanos) {
        VSyncMonitor.doFrame(nativePtr, frameTimeNanos);
      }
    });
  }

  @CalledByNative
  public static void requestOnUIThread(final long nativePtr) {
    if (sUIThreadChoreographer == null) {
      UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
        @Override
        public void run() {
          initUIThreadChoreographer();
          requestOnUIThread(nativePtr);
        }
      });
    } else {
      sUIThreadChoreographer.postFrameCallback(new Choreographer.FrameCallback() {
        @Override
        public void doFrame(long frameTimeNanos) {
          VSyncMonitor.doFrame(nativePtr, frameTimeNanos);
        }
      });
    }
  }

  private static void doFrame(long nativePtr, long frameTimeNanos) {
    long frameRefreshTimeNS = DEFAULT_FRAME_TIME_NS;
    try {
      WindowManager wm = mWindowManager.get();
      if (wm != null) {
        frameRefreshTimeNS = (long) (1000000000.0 / wm.getDefaultDisplay().getRefreshRate());
      }
    } catch (RuntimeException e) {
      // These code contains an inter-process communication, which may throw DeadSystemException.
      // And inside DisplayManagerGlobal, the DeadSystemException is wrapped into a RuntimeException
      // and rethrown, so we attempt to catch RuntimeException here.
      LLog.e("VSyncMonitor", "getRefreshRate failed: " + e.getMessage());
    }
    nativeOnVSync(nativePtr, frameTimeNanos, frameTimeNanos + frameRefreshTimeNS);
  }

  private static native void nativeOnVSync(
      long nativePtr, long frameStartTimeNS, long frameEndTimeNS);
}
