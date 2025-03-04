// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import android.util.Log;
import com.lynx.tasm.base.TraceEvent;

public class LynxTraceEnv {
  private static LynxTraceEnv sInstance;
  private volatile boolean mIsNativeLibraryLoaded = false;

  public static LynxTraceEnv inst() {
    if (sInstance == null) {
      synchronized (LynxTraceEnv.class) {
        if (sInstance == null) {
          sInstance = new LynxTraceEnv();
        }
      }
    }
    return sInstance;
  }

  private LynxTraceEnv() {}

  public boolean isNativeLibraryLoaded() {
    return mIsNativeLibraryLoaded;
  }

  public void markNativeLibraryLoaded(boolean status) {
    mIsNativeLibraryLoaded = status;
    TraceEvent.markTraceEnvInited(status);
  }

  public boolean init() {
    if (mIsNativeLibraryLoaded) {
      return mIsNativeLibraryLoaded;
    }
    mIsNativeLibraryLoaded = loadNativeTraceLibrary();
    TraceEvent.markTraceEnvInited(mIsNativeLibraryLoaded);
    return mIsNativeLibraryLoaded;
  }

  public boolean loadNativeTraceLibrary() {
    if (mIsNativeLibraryLoaded) {
      return mIsNativeLibraryLoaded;
    }
    try {
      System.loadLibrary("lynxtrace");
      return true;
    } catch (Exception e) {
      Log.e("trace env init", "failed to load liblynxtrace.so");
    }
    return false;
  }
}
