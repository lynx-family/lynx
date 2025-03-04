// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.framecapture;

import android.os.Build;
import android.os.SystemClock;
import android.view.Choreographer;

public class FrameTraceUtil {
  private static long mFrameIntervalNanos = (long) 1000000000 / 60;

  public static void addFrameCallback(final Runnable callback) {
    Choreographer.getInstance().postFrameCallback(new Choreographer.FrameCallback() {
      @Override
      public void doFrame(long frameTimeNanos) {
        callback.run();
      }
    });
  }

  public static long getSystemBootTimeNs() {
    if (Build.VERSION.SDK_INT > Build.VERSION_CODES.JELLY_BEAN) {
      return SystemClock.elapsedRealtimeNanos();
    }
    return SystemClock.elapsedRealtime() * 1000 * 1000;
  }

  public static long getFrameIntervalNanos() {
    return mFrameIntervalNanos;
  }
}
