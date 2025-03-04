// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.utils;

import android.text.TextUtils;
import androidx.annotation.Keep;
import com.lynx.tasm.LynxError;
import java.util.Map;

@Keep
public class ErrorUtils {
  private static final int CONSOLE_LOG_VERBOSE = -1;
  private static final int CONSOLE_LOG_INFO = 0;
  protected static final int CONSOLE_LOG_WARNING = 1;
  protected static final int CONSOLE_LOG_ERROR = 2;

  public static String getKeyMessage(LynxError error) {
    if (error == null || !error.isValid()) {
      return "";
    }
    StringBuilder sb = new StringBuilder();
    sb.append("code: ").append(error.getSubCode()).append("\n");
    sb.append("message: ").append(error.getSummaryMessage()).append("\n");
    String rootCause = error.getRootCause();
    if (!TextUtils.isEmpty(rootCause)) {
      sb.append("root_cause: ").append(error.getRootCause()).append("\n");
    }
    Map<String, String> contextInfo = error.getContextInfo();
    for (Map.Entry<String, String> i : contextInfo.entrySet()) {
      sb.append(i.getKey()).append(": ").append(i.getValue()).append("\n");
    }
    String fixSuggestion = error.getFixSuggestion();
    if (!TextUtils.isEmpty(fixSuggestion)) {
      sb.append("fix_suggestion: ").append(error.getFixSuggestion());
    }
    return sb.toString();
  }

  public static int errorLevelStrToInt(String levelStr) {
    int level = CONSOLE_LOG_ERROR;
    if (TextUtils.isEmpty(levelStr)) {
      return level;
    }
    if (levelStr != null && levelStr.equalsIgnoreCase(LynxError.LEVEL_WARN)) {
      level = CONSOLE_LOG_WARNING;
    }
    return level;
  }
}
