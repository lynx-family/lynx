// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.base;

import android.util.Log;
import com.lynx.base.IBaseNativeLibraryLoader;
import com.lynx.base.log.LynxLog;

public class LynxBaseEnv {
  private static LynxBaseEnv sInstance;
  private volatile boolean mIsNativeLibraryLoaded = false;

  public static LynxBaseEnv inst() {
    if (sInstance == null) {
      synchronized (LynxBaseEnv.class) {
        if (sInstance == null) {
          sInstance = new LynxBaseEnv();
        }
      }
    }
    return sInstance;
  }

  private LynxBaseEnv() {}

  public boolean isNativeLibraryLoaded() {
    return mIsNativeLibraryLoaded;
  }

  public boolean init(
      IBaseNativeLibraryLoader nativeLibraryLoader, boolean isPrintLogsToAllChannels) {
    if (!mIsNativeLibraryLoaded) {
      mIsNativeLibraryLoaded = loadNativeTraceLibrary(nativeLibraryLoader);
    }
    LynxLog.initLynxLog(isPrintLogsToAllChannels);
    return true;
  }

  public synchronized boolean loadNativeTraceLibrary(IBaseNativeLibraryLoader nativeLibraryLoader) {
    if (mIsNativeLibraryLoaded) {
      return mIsNativeLibraryLoaded;
    }
    try {
      if (nativeLibraryLoader != null) {
        nativeLibraryLoader.loadLibrary("lynxbase");
      } else {
        System.loadLibrary("lynxbase");
      }
      return true;
    } catch (Exception e) {
      Log.e("lynx base env init", "failed to load liblynxbase.so");
      return false;
    }
  }
}
