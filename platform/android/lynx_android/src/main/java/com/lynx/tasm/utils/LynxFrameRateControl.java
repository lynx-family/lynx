// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import android.annotation.SuppressLint;
import android.os.Build;
import android.util.Log;
import android.view.Choreographer;
import java.lang.ref.WeakReference;

public class LynxFrameRateControl {
  private WeakReference<VSyncListener> mListener;
  private static final long VSYNC_FRAME = 1000 / 16;
  private Choreographer mChoreographer;
  private final Choreographer.FrameCallback mVSyncFrameCallback;
  private boolean mRunning;
  private boolean mPreState;

  public interface VSyncListener {
    void OnVSync(long frameTimeNanos);
  }

  public LynxFrameRateControl(VSyncListener listener) {
    mRunning = false;
    mListener = new WeakReference<>(listener);
    if (Build.VERSION.SDK_INT > Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1) {
      try {
        mChoreographer = Choreographer.getInstance();
      } catch (Throwable e) {
        Log.e("lynx", "Choreographer.getInstance got exception");
      }
      mVSyncFrameCallback = new Choreographer.FrameCallback() {
        @SuppressLint("NewApi")
        @Override
        public void doFrame(long frameTimeNanos) {
          VSyncListener vSyncListener;
          if (mListener != null && (vSyncListener = mListener.get()) != null) {
            try {
              vSyncListener.OnVSync(frameTimeNanos);
              mChoreographer.postFrameCallback(mVSyncFrameCallback);
            } catch (Throwable e) {
              Log.e("LynxFrameRateControl", "VSync callback exception:" + e.toString());
            }
          }
        }
      };
    } else {
      mChoreographer = null;
      mVSyncFrameCallback = null;
    }
  }

  @SuppressLint("NewApi")
  public void start() {
    if (!mRunning) {
      if (mChoreographer != null) {
        try {
          mChoreographer.postFrameCallback(mVSyncFrameCallback);
        } catch (Throwable e) {
          Log.e("LynxFrameRateControl", "VSync postFrameCallback exception:" + e.toString());
        }
      }
      Log.d("LynxFrameRateControl", "real start");
      mRunning = true;
    }
  }

  @SuppressLint("NewApi")
  public void stop() {
    Log.d("LynxFrameRateControl", "stop");

    if (mChoreographer != null) {
      try {
        mChoreographer.removeFrameCallback(mVSyncFrameCallback);
      } catch (Throwable e) {
        Log.e("LynxFrameRateControl", "VSync removeFrameCallback exception:" + e.toString());
      }
    }
    mRunning = false;
  }

  public void onScreenOff() {
    mPreState = mRunning;
    if (mRunning) {
      stop();
    }
  }

  public void onScreenOn() {
    if (mPreState) {
      start();
    }
  }
}
