// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import android.util.Log;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.base.CalledByNative;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.lang.Throwable;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.Set;

public class CallStackUtil {
  // We limit this length to avoid bad impact to uploading process.
  // Also it reduces data traffic load.
  private static final int MAX_LENGTH = 900;
  private static final int MAX_LINE_LENGTH = 9;
  private static final int MAX_CAUSE_NUM_IN_MESSAGE = 5;

  /**
   * Get message of a throwable and its cause chain. The length of
   * the returned cause chain does not exceed MAX_CAUSE_NUM_IN_MESSAGE.
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  @CalledByNative
  public static String getMessageOfCauseChain(Throwable t) {
    if (t == null) {
      return "";
    }
    // After each JNI invocation, a check is performed to determine if any exception occurred
    // during the invocation. If an exception occurs, this method is called to get the message
    // of the exception. However, since this method itself is also invoked by JNI, it is
    // necessary to use try-catch to prevent infinite recursion.
    try {
      StringBuilder builder = new StringBuilder();
      builder.append(t);
      int index = MAX_CAUSE_NUM_IN_MESSAGE;
      Throwable cause = t.getCause();
      while (cause != null && index > 0) {
        index--;
        builder.append(": ").append(cause);
        cause = cause.getCause();
      }
      return builder.toString();
    } catch (Throwable e) {
      return t.toString();
    }
  }

  /**
   *  Get stack of a throwable and its cause chain, with a maximum number of
   *  total characters not exceeding MAX_LENGTH.
   */
  public static String getStackTraceStringTrimmed(Throwable t) {
    String stack = Log.getStackTraceString(t);
    if (stack.length() > MAX_LENGTH) {
      stack = stack.substring(0, MAX_LENGTH);
    }
    return stack;
  }

  /**
   *  Get stack of a throwable and its cause chain, with a maximum number of
   *  stack trace lines of each cause not exceeding MAX_LINE_LENGTH.
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  @CalledByNative
  public static String getStackTraceStringWithLineTrimmed(Throwable t) {
    if (t == null) {
      return "";
    }
    // After each JNI invocation, a check is performed to determine if any exception occurred
    // during the invocation. If an exception occurs, this method is called to retrieve the
    // stack trace of the exception. However, since this method itself is also invoked by JNI,
    // it is necessary to use try-catch to prevent infinite recursion.
    try {
      StringWriter sw = new StringWriter();
      PrintWriter pw = new PrintWriter(sw);
      printStackTraceWithLineTrimmed(t, pw);
      pw.flush();
      return sw.toString();
    } catch (Throwable e) {
      return getStackTraceStringTrimmed(t);
    }
  }

  /**
   * Print stack of a throwable and its cause chain to a PrintWriter.
   */
  private static void printStackTraceWithLineTrimmed(Throwable t, PrintWriter pw) {
    Set<Throwable> throwablePrinted =
        Collections.newSetFromMap(new IdentityHashMap<Throwable, Boolean>());
    printStackTraceWithLineTrimmed(t, throwablePrinted, pw);
  }

  /**
   * Print stack of a throwable and its cause chain to a PrintWriter. Parameter
   * throwablePrinted is used to check whether a throwable has been printed
   * before, and throwablePrinted cannot be null.
   */
  private static void printStackTraceWithLineTrimmed(
      Throwable t, Set<Throwable> throwablePrinted, PrintWriter pw) {
    if (t == null || pw == null || throwablePrinted == null || throwablePrinted.contains(t)) {
      return;
    }
    if (!throwablePrinted.isEmpty()) {
      pw.print("Caused by: ");
    }
    pw.println(t);
    throwablePrinted.add(t);

    StackTraceElement[] trace = t.getStackTrace();
    int length = trace.length < MAX_LINE_LENGTH ? trace.length : MAX_LINE_LENGTH;
    for (int i = 0; i < length; i++) {
      pw.println("\tat " + trace[i]);
    }

    printStackTraceWithLineTrimmed(t.getCause(), throwablePrinted, pw);
  }
}
