// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base;

@Deprecated
public interface ILogDelegate {
  /**
   * delegate type
   * @return
   */
  int type();
  /**
   * Sets a minimum log-level under which the logger will not log regardless of other checks.
   *
   * @param level the minimum level to set
   */
  void setMinimumLoggingLevel(int level);

  /**
   * Gets a minimum log-level under which the logger will not log regardless of other checks.
   *
   * @return the minimum level
   */
  int getMinimumLoggingLevel();

  /**
   * Gets whether the specified level is loggable.
   *
   * @param level the level to check
   * @return the level
   */
  boolean isLoggable(int level);

  boolean isLoggable(LogSource source, int level);

  /**
   * Send a {@link android.util.Log#VERBOSE} log message.
   *
   * @param tag Used to identify the source of a log message.  It usually identifies
   *            the class or activity where the log call occurs.
   * @param msg The message you would like logged.
   */
  void v(String tag, String msg);

  /**
   * Send a {@link android.util.Log#DEBUG} log message.
   *
   * @param tag Used to identify the source of a log message.  It usually identifies
   *            the class or activity where the log call occurs.
   * @param msg The message you would like logged.
   */
  void d(String tag, String msg);

  /**
   * Send an {@link android.util.Log#INFO} log message.
   *
   * @param tag Used to identify the source of a log message.  It usually identifies
   *            the class or activity where the log call occurs.
   * @param msg The message you would like logged.
   */
  void i(String tag, String msg);

  /**
   * Send a {@link android.util.Log#WARN} log message.
   *
   * @param tag Used to identify the source of a log message.  It usually identifies
   *            the class or activity where the log call occurs.
   * @param msg The message you would like logged.
   */
  void w(String tag, String msg);

  /**
   * Send an {@link android.util.Log#ERROR} log message.
   *
   * @param tag Used to identify the source of a log message.  It usually identifies
   *            the class or activity where the log call occurs.
   * @param msg The message you would like logged.
   */
  void e(String tag, String msg);

  /**
   * Send an key message.
   *
   * @param tag Used to identify the source of a log message.  It usually identifies
   *            the class or activity where the log call occurs.
   * @param msg The message you would like logged.
   */
  void k(String tag, String msg);

  /**
   * Logs a message.
   *
   * @param priority the priority of the message
   * @param tag Used to identify the source of a log message.
   * @param msg The message you would like logged.
   */
  void log(int priority, String tag, String msg);
}
