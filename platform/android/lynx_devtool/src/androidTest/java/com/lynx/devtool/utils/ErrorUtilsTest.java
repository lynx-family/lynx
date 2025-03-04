// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.utils;

import static org.junit.Assert.*;

import com.lynx.tasm.LynxError;
import org.junit.Test;

public class ErrorUtilsTest {
  @Test
  public void getKeyMessage() {
    // test error with message
    LynxError errorWithMessage = new LynxError(1, "test", "test", "test");
    String keyMessage = ErrorUtils.getKeyMessage(errorWithMessage);
    assertEquals("code: 1\nmessage: test\nfix_suggestion: test", keyMessage);

    // test error is null
    String keyMessageNull = ErrorUtils.getKeyMessage(null);
    assertEquals("", keyMessageNull);

    // test error is invalid
    LynxError errorInvalid = new LynxError(1, "", "", "");
    String keyMessageInvalid = ErrorUtils.getKeyMessage(errorInvalid);
    assertEquals("", keyMessageInvalid);
  }

  @Test
  public void errorLevelStrToInt() {
    // test error level is empty
    int levelEmpty = ErrorUtils.errorLevelStrToInt("");
    assertEquals(ErrorUtils.CONSOLE_LOG_ERROR, levelEmpty);

    // test error level is warn
    int levelWarn = ErrorUtils.errorLevelStrToInt(LynxError.LEVEL_WARN);
    assertEquals(ErrorUtils.CONSOLE_LOG_WARNING, levelWarn);

    // test error level is error
    int levelError = ErrorUtils.errorLevelStrToInt(LynxError.LEVEL_ERROR);
    assertEquals(ErrorUtils.CONSOLE_LOG_ERROR, levelError);
  }
}
