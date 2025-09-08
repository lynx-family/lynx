// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import androidx.annotation.Nullable;
import com.lynx.base.log.LynxLog;
import com.lynx.tasm.LynxEnv;

public class LLog {
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

  public static void initLynxLog() {
    LynxLog.initLynxLog(LynxEnv.inst().isDevtoolEnabled());
  }

  public static void setDebugLoggingDelegate(AbsLogDelegate delegate) {
    BaseLogDelegate.inst().setDelegate(delegate);
    LynxLog.setDebugLoggingDelegate(BaseLogDelegate.inst());
  }

  public static void setMinimumLoggingLevel(int level) {
    LynxLog.setMinimumLoggingLevel(level);
  }

  public static int getMinimumLoggingLevel() {
    return LynxLog.getMinimumLoggingLevel();
  }

  /*
   * turn off by default
   * JS logs form external channels: recorded by business developers (mostly front-end)
   */
  public static void setJSLogsFromExternalChannels(boolean isOpen) {
    LynxLog.setJSLogsFromExternalChannels(isOpen);
  }

  public static void v(String tag, String msg) {
    LynxLog.v(tag, msg);
  }

  public static void d(String tag, String msg) {
    LynxLog.d(tag, msg);
  }

  public static void i(String tag, String msg) {
    LynxLog.i(tag, msg);
  }

  public static void w(String tag, String msg) {
    LynxLog.w(tag, msg);
  }

  public static void e(String tag, String msg) {
    LynxLog.e(tag, msg);
  }

  /**
   * @deprecated LLog no longer supports REPORT level;
   *             LLog.e(String tag, String msg) is recommended.
   */
  @Deprecated
  public static void report(String tag, String msg) {
    LynxLog.e(tag, msg);
  }

  public static void internalLog(int level, String tag, String msg) {
    LynxLog.internalLog(level, tag, msg);
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
    LynxLog.DCHECK(condition);
  }

  public static void DTHROW() {
    DTHROW(null);
  }

  public static void DTHROW(@Nullable RuntimeException e) {
    LynxLog.DTHROW(e);
  }

  @Deprecated
  public static void initALog(long addr) {}

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
