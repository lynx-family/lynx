// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

import android.util.Log;

public abstract class AbsLogDelegate implements ILogDelegate {
  public final static int TYPE_OVERRIDE = 1; // type override for log implementation
  public final static int TYPE_INC = 2; // type increment for log implementation
  public int mMinimumLoggingLevel = Log.INFO;
  /**
   * Indicates log delegate type
   * @return
   */
  public int type() {
    return TYPE_INC;
  }
  /**
   * Sets a minimum log-level under which the logger will not log regardless of other checks.
   *
   * @param level the minimum level to set
   */
  public void setMinimumLoggingLevel(int level) {
    mMinimumLoggingLevel = level;
  }

  /**
   * Gets a minimum log-level under which the logger will not log regardless of other checks.
   *
   * @return the minimum level
   */
  public int getMinimumLoggingLevel() {
    return mMinimumLoggingLevel;
  }

  /**
   * Gets whether the specified level is loggable.
   *
   * @param level the level to check
   * @return the level
   */
  public boolean isLoggable(int level) {
    return mMinimumLoggingLevel <= level;
  }

  /**
   * Gets whether the specified level and the specified source is loggable.
   * @param source  the log source to check
   * @param level   the level to check
   * @return
   */
  public boolean isLoggable(LogSource source, int level) {
    return mMinimumLoggingLevel <= level;
  }

  /**
   * Send a {@link android.util.Log#VERBOSE} log message.
   *
   * @param tag Used to identify the source of a log message.  It usually identifies
   *            the class or activity where the log call occurs.
   * @param msg The message you would like logged.
   */
  public void v(String tag, String msg) {
    println(Log.VERBOSE, tag, msg);
  }

  /**
   * Send a {@link android.util.Log#DEBUG} log message.
   *
   * @param tag Used to identify the source of a log message.  It usually identifies
   *            the class or activity where the log call occurs.
   * @param msg The message you would like logged.
   */
  public void d(String tag, String msg) {
    println(Log.DEBUG, tag, msg);
  }

  /**
   * Send an {@link android.util.Log#INFO} log message.
   *
   * @param tag Used to identify the source of a log message.  It usually identifies
   *            the class or activity where the log call occurs.
   * @param msg The message you would like logged.
   */
  public void i(String tag, String msg) {
    println(Log.INFO, tag, msg);
  }

  /**
   * Send a {@link android.util.Log#WARN} log message.
   *
   * @param tag Used to identify the source of a log message.  It usually identifies
   *            the class or activity where the log call occurs.
   * @param msg The message you would like logged.
   */
  public void w(String tag, String msg) {
    println(Log.WARN, tag, msg);
  }

  /**
   * Send an {@link android.util.Log#ERROR} log message.
   *
   * @param tag Used to identify the source of a log message.  It usually identifies
   *            the class or activity where the log call occurs.
   * @param msg The message you would like logged.
   */
  public void e(String tag, String msg) {
    println(Log.ERROR, tag, msg);
  }

  /**
   * Logs a message.
   *
   * @param priority the priority of the message
   * @param tag Used to identify the source of a log message.
   * @param msg The message you would like logged.
   */
  public void log(int priority, String tag, String msg) {
    println(priority, tag, msg);
  }

  private void println(int priority, String tag, String msg) {
    if (tag != null && msg != null) {
      Log.println(priority, tag, msg);
    }
  }

  public void k(String tag, String msg) {}
}
