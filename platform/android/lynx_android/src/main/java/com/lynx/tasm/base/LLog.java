// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import android.os.Build;
import android.util.Log;
import androidx.annotation.Nullable;
import com.lynx.BuildConfig;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.service.ILynxLogService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ConcurrentHashMap;

public class LLog {
  private static final String TAG = "LLog";

  /*
   * REPORT is deprecated
   * */
  @Deprecated public static final int REPORT = -1;

  /*
   * Align LLog levels to the LogSeverity in the <logging.h>
   * */
  public static final int VERBOSE = 0;
  public static final int DEBUG = 1;
  public static final int INFO = 2;
  public static final int WARN = 3;
  public static final int ERROR = 4;

  private static AbsLogDelegate sDebugLoggingDelegate;
  private static int sALogMinLogLevel = BuildConfig.DEBUG ? DEBUG : INFO;
  private static long alogNativePtr = 0;

  private static boolean sIsNativeLibLoad = false;
  private static final int sDefaultRuntimeId = -1;
  private static boolean sIsJSLogsFromExternalChannelsOpen = false;
  private static ILynxLogService service = null;
  private enum LogChannel { INTERNAL, EXTERNAL }

  public static void initLynxLog() {
    try {
      if (!sIsNativeLibLoad) {
        sIsNativeLibLoad = LynxEnv.inst().isNativeLibraryLoaded();
      }
      if (sIsNativeLibLoad) {
        initLynxLogging();
      }
    } catch (ArrayIndexOutOfBoundsException error) {
      Log.e("lynx", "init LynxLogging exception [ " + error.getMessage() + " ]");
    }
  }

  private static void initLynxLogging() {
    nativeInitLynxLoggingNative();
    detectALogDependence();
    setLogOutputChannel();
  }

  public static void setDebugLoggingDelegate(AbsLogDelegate delegate) {
    sDebugLoggingDelegate = delegate;
  }

  public static void setMinimumLoggingLevel(int level) {
    try {
      if (!sIsNativeLibLoad) {
        sIsNativeLibLoad = LynxEnv.inst().isNativeLibraryLoaded();
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

  private static boolean isExternalChannel(LogChannel channelType) {
    return sIsJSLogsFromExternalChannelsOpen && channelType.equals(LogChannel.EXTERNAL);
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

  /**
   * @deprecated LLog no longer supports REPORT level;
   *             LLog.e(String tag, String msg) is recommended.
   */
  @Deprecated
  public static void report(String tag, String msg) {
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

  private static void logByDebugLoggingDelegate(int level, String tag, String msg) {
    logByDebugLoggingDelegate(level, tag, msg, sDefaultRuntimeId);
  }

  /**
   * upload logs to devtool for debug mode
   */
  private static void logByDebugLoggingDelegate(int level, String tag, String msg, long runtimeId) {
    if (sDebugLoggingDelegate == null || !LynxEnv.inst().isLynxDebugEnabled()) {
      return;
    }
    String msgWithRid = msg;
    if (runtimeId != sDefaultRuntimeId) {
      msgWithRid = "argRuntimeId:" + runtimeId + "&" + msg;
    }
    switch (level) {
      case VERBOSE:
        sDebugLoggingDelegate.v(tag, msgWithRid);
        break;
      case DEBUG:
        sDebugLoggingDelegate.d(tag, msgWithRid);
        break;
      case INFO:
        sDebugLoggingDelegate.i(tag, msgWithRid);
        break;
      case WARN:
        sDebugLoggingDelegate.w(tag, msgWithRid);
        break;
      case ERROR:
        sDebugLoggingDelegate.e(tag, msgWithRid);
        break;
    }
  }

  public static void internalLog(int level, String tag, String msg) {
    if (msg == null || tag == null) {
      return;
    }
    logByDebugLoggingDelegate(level, tag, msg);
    /**
     * TODO(shiwentao):
     * In case of multi-thread, command reordering will make
     * mIsNativeLibraryLoaded(LynxEnv.java) true before liblynx.so has be loaded. 2.7 will
     * be fixed at LynxEnv.java
     */
    try {
      if (!sIsNativeLibLoad) {
        sIsNativeLibLoad = LynxEnv.inst().isNativeLibraryLoaded();
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
      Log.e("lynx", "load liblynx.so exception [ " + e.getMessage() + " ]");
    }
  }

  /**
   * @deprecated
   *             By default, All Lynx logs are for internal use only.
   */
  public static void internalLog(
      int level, String tag, String msg, LogSource source, Long runtimeId, int messageStart) {
    // TODO(lipin.1001): The JavaScript logs need to be processed from outside Lynx.
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

  private static int convertLLogLevelToLogLevel(int level) {
    switch (level) {
      case VERBOSE:
        return Log.VERBOSE;
      case DEBUG:
        return Log.DEBUG;
      case INFO:
        return Log.INFO;
      case WARN:
        return Log.WARN;
      case ERROR:
        return Log.ERROR;
      default:
        return Log.ASSERT;
    }
  }

  private static void detectALogDependence() {
    long address = 0;
    service = LynxServiceCenter.inst().getService(ILynxLogService.class);
    if (service != null) {
      address = service.getDefaultWriteFunction();
    }
    if (address != 0) {
      initALog(address);
      Log.i(TAG, "LynxLog dependency load successfully. function native address is " + address);
      return;
    }
    Log.i(TAG, "failed to load LynxLog dependency");
  }

  private static void setLogOutputChannel() {
    if (service != null && service.isLogOutputByPlatform()) {
      nativeSetLogOutputByPlatform();
    }
  }

  public static void initALog(long addr) {
    alogNativePtr = addr;
    nativeInitALogNative(addr);
  }

  private static native void nativeSetNativeMinLogLevel(int level);
  private static native void nativeInitALogNative(long addr);
  private static native void nativeInternalLog(int level, String tag, String msg);
  private static native void nativeInitLynxLoggingNative();
  private static native void nativeSetLogOutputByPlatform();

  private static void logJS(int priority, String tag, String msg, int source, long runtimeId,
      int channelType, int messageStart) {
    LogSource sourceType = LogSource.values()[source];
    switch (sourceType) {
      case JS:
        if (isExternalChannel(LogChannel.values()[channelType])) {
          internalLog(priority, tag, msg, sourceType, runtimeId, messageStart);
        }
        return;
      case JS_EXT:
        if (priority == ERROR) {
          internalLog(priority, tag, msg, sourceType, runtimeId, messageStart);
        }
        return;
      default:
        break;
    }
  }

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
      logByDebugLoggingDelegate(priority, tag, msg, runtimeId);
      // 2.only upload external JS logs and console.report to logging delegate
      logJS(priority, tag, msg, source, runtimeId, channelType, messageStart);
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

  @CalledByNative
  private static long getALogPtr() {
    return alogNativePtr;
  }

  // deprecated functions
  @Deprecated
  public static void setLoggingDelegate(AbsLogDelegate delegate) {}

  @Deprecated
  public static boolean isLoggable(int level) {
    return false;
  }

  @Deprecated
  public static int addLoggingDelegate(AbsLogDelegate delegate) {
    return -1;
  }

  @Deprecated
  public static synchronized void removeLoggingDelegate(int delegateId) {}

  @Deprecated
  public static void onEnvReady() {}
}
