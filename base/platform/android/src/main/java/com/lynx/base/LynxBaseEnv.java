// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.base;

import android.content.Context;
import android.util.Log;
import com.lynx.base.IBaseNativeLibraryLoader;
import com.lynx.base.LynxBaseTrace;
import com.lynx.base.log.LynxLog;
import java.io.File;
import java.nio.charset.StandardCharsets;

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
    return init(null, nativeLibraryLoader, isPrintLogsToAllChannels);
  }

  public boolean init(Context context, IBaseNativeLibraryLoader nativeLibraryLoader,
      boolean isPrintLogsToAllChannels) {
    if (!mIsNativeLibraryLoaded) {
      mIsNativeLibraryLoaded = loadNativeTraceLibrary(nativeLibraryLoader);
    }
    if (mIsNativeLibraryLoaded && context != null) {
      File directory = context.getFilesDir();
      if (directory != null) {
        nativeSetApplicationDataDirectory(directory.getAbsolutePath());
      }
    }
    LynxLog.initLynxLog(isPrintLogsToAllChannels);
    LynxBaseTrace.init();
    return true;
  }

  /** Returns the effective native application data directory, or empty before initialization. */
  public String getApplicationDataDirectory() {
    return mIsNativeLibraryLoaded
        ? new String(nativeGetApplicationDataDirectory(), StandardCharsets.UTF_8)
        : "";
  }

  /** Returns a child directory, creating it if needed, or empty if unavailable. */
  public String getApplicationDataSubDirectory(String name) {
    return mIsNativeLibraryLoaded
        ? new String(nativeGetApplicationDataSubDirectory(name), StandardCharsets.UTF_8)
        : "";
  }

  /** Overrides the complete directory after base initialization and before consumers start. */
  public void setApplicationDataDirectory(String directory) {
    nativeSetApplicationDataDirectory(directory);
  }

  private static native byte[] nativeGetApplicationDataDirectory();
  private static native byte[] nativeGetApplicationDataSubDirectory(String name);
  private static native void nativeSetApplicationDataDirectory(String directory);

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
