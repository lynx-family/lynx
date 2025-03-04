// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static org.junit.Assert.*;

import org.junit.Test;

public class CallStackUtilTest {
  private final static String EXCEPTION_MSG_1 = "throwable without any cause";
  private final static String EXCEPTION_MSG_2 = "throwable message 2";
  private final static String EXCEPTION_MSG_3 = "throwable message 3";
  private final static String EXCEPTION_MSG_4 = "throwable message 4";
  private final static String JAVA_EXCEPTION_PREFIX = "java.lang.Exception: ";
  private final static String CAUSE_CAPTION = "Caused by: ";

  @Test
  public void getMessageOfCauseChain() {
    // test pass a null throwable
    String message = CallStackUtil.getMessageOfCauseChain(null);
    assertTrue(message.isEmpty());

    // test get message from a throwable without any cause
    Exception e = new Exception(EXCEPTION_MSG_1);
    message = CallStackUtil.getMessageOfCauseChain(e);
    assertEquals("java.lang.Exception: " + EXCEPTION_MSG_1, message);

    // test get message from a throwable with a cause chain
    Exception e2 = new Exception(EXCEPTION_MSG_2);
    Exception e3 = new Exception(EXCEPTION_MSG_3, e2);
    Exception e4 = new Exception(EXCEPTION_MSG_4, e3);
    message = CallStackUtil.getMessageOfCauseChain(e4);
    String expectMessage = JAVA_EXCEPTION_PREFIX + EXCEPTION_MSG_4 + ": " + JAVA_EXCEPTION_PREFIX
        + EXCEPTION_MSG_3 + ": " + JAVA_EXCEPTION_PREFIX + EXCEPTION_MSG_2;
    assertEquals(expectMessage, message);

    // test get message from a circular exception chain
    e2.initCause(e3);
    message = CallStackUtil.getMessageOfCauseChain(e4);
    expectMessage = expectMessage + ": " + JAVA_EXCEPTION_PREFIX + EXCEPTION_MSG_3 + ": "
        + JAVA_EXCEPTION_PREFIX + EXCEPTION_MSG_2 + ": " + JAVA_EXCEPTION_PREFIX + EXCEPTION_MSG_3;
    assertEquals(expectMessage, message);
  }

  @Test
  public void getStackTraceStringWithLineTrimmed() {
    // test pass a null throwable
    String message = CallStackUtil.getStackTraceStringWithLineTrimmed(null);
    assertTrue(message.isEmpty());

    // test pass a throwable without any cause
    Exception e1 = new Exception(EXCEPTION_MSG_1);
    String message1 = CallStackUtil.getStackTraceStringWithLineTrimmed(e1);

    // test pass a throwable with a cause
    Exception e2 = new Exception(EXCEPTION_MSG_2, e1);
    String message2 = CallStackUtil.getStackTraceStringWithLineTrimmed(e2);

    int index = message2.indexOf(CAUSE_CAPTION);
    assertNotEquals(-1, index);
    assertEquals(message1, message2.substring(index + CAUSE_CAPTION.length()));

    // test get stack trace of a circular exception chain
    e1.initCause(e2);
    String message3 = CallStackUtil.getStackTraceStringWithLineTrimmed(e2);
    assertEquals(message2, message3);
  }
}
