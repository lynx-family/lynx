// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.base.log;

import android.util.Log;
import androidx.annotation.Nullable;
import com.lynx.base.BuildConfig;
import com.lynx.base.CalledByNative;
import com.lynx.base.LynxBaseEnv;
import com.lynx.tasm.service.ILynxLogService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.Timer;
import java.util.TimerTask;

public class LynxLog {
  private static final String TAG = "LynxLog";

  /*
   * Align LLog levels to the LogSeverity in the <logging.h>
   * */
  public static final int VERBOSE = 0;
  public static final int DEBUG = 1;
  public static final int INFO = 2;
  public static final int WARN = 3;
  public static final int ERROR = 4;

  private static AbsBaseLogDelegate sDebugLoggingDelegate;
  private static int sALogMinLogLevel = BuildConfig.DEBUG ? DEBUG : INFO;

  private static boolean sIsNativeLibLoad = false;
  private static boolean sIsJSLogsFromExternalChannelsOpen = false;
  private static ILynxLogService service = null;

  // set interval as 500 ms when init LynxLog
  private static final int sDetectALogDependencyInterval = 500;
  private static int sTryCounts = 0;
  private static final int sMaxTryCounts = (1 * 60 * 1000) / sDetectALogDependencyInterval;

  public static void initLynxLog(boolean isPrintLogsToAllChannels) {
    try {
      if (!sIsNativeLibLoad) {
        sIsNativeLibLoad = LynxBaseEnv.inst().isNativeLibraryLoaded();
      }
      if (sIsNativeLibLoad) {
        initLynxLogging(isPrintLogsToAllChannels);
      }
    } catch (ArrayIndexOutOfBoundsException error) {
      Log.e("lynx", "init LynxLogging exception [ " + error.getMessage() + " ]");
    }
  }

  private static void initLynxLogging(boolean isPrintLogsToAllChannels) {
    nativeInitLynxLoggingNative(isPrintLogsToAllChannels);
    asyncInitLynxNativeLog();
    setLogOutputChannel();
  }

  public static void setDebugLoggingDelegate(AbsBaseLogDelegate delegate) {
    sDebugLoggingDelegate = delegate;
  }

  public static void setMinimumLoggingLevel(int level) {
    try {
      if (!sIsNativeLibLoad) {
        sIsNativeLibLoad = LynxBaseEnv.inst().isNativeLibraryLoaded();
      }
      if (sIsNativeLibLoad) {
        final String[] logLevelName = {"VERBOSE", "DEBUG", "INFO", "WARN", "ERROR"};
        if (sALogMinLogLevel < level) {
          sALogMinLogLevel = level;
          nativeSetNativeMinLogLevel(level);
          Log.w("lynx",
              String.format("Reset minimum log level as %s", logLevelName[sALogMinLogLevel]));
        } else {
          Log.w("lynx",
              String.format("Please set a log level higher than %s to filter lynx logs!",
                  logLevelName[sALogMinLogLevel]));
        }
      }
    } catch (ArrayIndexOutOfBoundsException error) {
      Log.e("lynx", "Please check index, " + error.getMessage());
    }
  }

  public static int getMinimumLoggingLevel() {
    return sALogMinLogLevel;
  }

  /*
   * turn off by default
   * JS logs form external channels: recorded by business developers (mostly front-end)
   */
  public static void setJSLogsFromExternalChannels(boolean isOpen) {
    sIsJSLogsFromExternalChannelsOpen = isOpen;
  }

  public static void v(String tag, String msg) {
    internalLog(VERBOSE, tag, msg);
  }

  public static void d(String tag, String msg) {
    internalLog(DEBUG, tag, msg);
  }

  public static void i(String tag, String msg) {
    internalLog(INFO, tag, msg);
  }

  public static void w(String tag, String msg) {
    internalLog(WARN, tag, msg);
  }

  public static void e(String tag, String msg) {
    internalLog(ERROR, tag, msg);
  }

  private static void logByAndroidUtil(int level, String tag, String msg) {
    switch (level) {
      case VERBOSE:
        Log.v(tag, msg);
        break;
      case DEBUG:
        Log.d(tag, msg);
        break;
      case INFO:
        Log.i(tag, msg);
        break;
      case WARN:
        Log.w(tag, msg);
        break;
      case ERROR:
        Log.e(tag, msg);
        break;
    }
  }

  /**
   * upload logs to devtool for debug mode
   */
  private static void logByDebugLoggingDelegate(int level, String tag, String msg) {
    if (sDebugLoggingDelegate == null) {
      return;
    }
    switch (level) {
      case VERBOSE:
        sDebugLoggingDelegate.v(tag, msg);
        break;
      case DEBUG:
        sDebugLoggingDelegate.d(tag, msg);
        break;
      case INFO:
        sDebugLoggingDelegate.i(tag, msg);
        break;
      case WARN:
        sDebugLoggingDelegate.w(tag, msg);
        break;
      case ERROR:
        sDebugLoggingDelegate.e(tag, msg);
        break;
    }
  }

  public static void internalLog(int level, String tag, String msg) {
    if (msg == null || tag == null) {
      return;
    }
    logByDebugLoggingDelegate(level, tag, msg);
    try {
      if (!sIsNativeLibLoad) {
        sIsNativeLibLoad = LynxBaseEnv.inst().isNativeLibraryLoaded();
        if (!sIsNativeLibLoad) {
          logByAndroidUtil(level, tag, msg);
          return;
        }
      }
      if (level >= sALogMinLogLevel) {
        if (service != null && service.isLogOutputByPlatform()) {
          service.logByPlatform(level, tag, msg);
        } else {
          nativeInternalLog(level, tag, msg);
        }
      }
    } catch (UnsatisfiedLinkError e) {
      // in the case of only ALog delegation, So here don`t use LLog
      Log.e("lynx", "load liblynxbase.so exception [ " + e.getMessage() + " ]");
    }
  }

  public static void DCHECK(boolean condition) {
    if (!BuildConfig.DEBUG) {
      return;
    }
    if (!condition) {
      throw new RuntimeException("LYNX DEBUG ABORT");
    }
  }

  public static void DTHROW() {
    DTHROW(null);
  }

  public static void DTHROW(@Nullable RuntimeException e) {
    if (!BuildConfig.DEBUG) {
      return;
    }
    if (e != null) {
      throw e;
    } else {
      throw new RuntimeException("LYNX DEBUG ABORT");
    }
  }

  private static void asyncInitLynxNativeLog() {
    if (detectALogDependence()) {
      return;
    }
    final Timer timer = new Timer();
    TimerTask timerTask = new TimerTask() {
      @Override
      public void run() {
        if (detectALogDependence()) {
          cancel();
          timer.cancel();
        }
      }
    };
    timer.schedule(timerTask, 0, sDetectALogDependencyInterval);
  }

  private static boolean detectALogDependence() {
    long address = 0;
    service = LynxServiceCenter.inst().getService(ILynxLogService.class);
    if (service != null) {
      address = service.getDefaultWriteFunction();
    }
    if (address != 0) {
      nativeInitALogNative(address);
      Log.i(TAG, "LynxLog dependency load successfully. function native address is " + address);
      return true;
    }
    if (++sTryCounts >= sMaxTryCounts) {
      Log.i(TAG, "failed to load LynxLog dependency");
      return true;
    }
    return false;
  }

  private static void setLogOutputChannel() {
    if (service != null && service.isLogOutputByPlatform()) {
      nativeSetLogOutputByPlatform();
    }
  }

  private static native void nativeSetNativeMinLogLevel(int level);
  private static native void nativeInitALogNative(long addr);
  private static native void nativeInternalLog(int level, String tag, String msg);
  private static native void nativeInitLynxLoggingNative(boolean isPrintLogsToAllChannels);
  private static native void nativeSetLogOutputByPlatform();

  @CalledByNative
  private static void log(int priority, String tag, String msg, int source, long runtimeId,
      int channelType, int messageStart) {
    try {
      priority = priority > ERROR ? ERROR : priority;
      // 0.consume all logs from the native layer.
      if (service != null && service.isLogOutputByPlatform()) {
        service.logByPlatform(priority, tag, msg);
      }
      // 1.upload all logs to devtool for debug mode
      logByDebugLoggingDelegate(priority, tag, msg);
    } catch (Throwable e) {
      Log.e("lynx", "" + e.getMessage());
    }
  }

  /**
   * Emoji will make App crash when use `NewStringUTF` API in Android 5.x
   */
  @CalledByNative
  private static void logByte(int priority, String tag, byte[] msg, int source, long runtimeId,
      int channelType, int messageStart) {
    log(priority, tag, new String(msg), source, runtimeId, channelType, messageStart);
  }
}
