// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.logbox;

import static org.junit.Assert.*;

import org.junit.Test;

public class LynxLogBoxManagerTest {
  // origin messages
  static final String NATIVE_ERROR =
      "{\"sdk\":\"1.0.0\",\"level\":\"error\",\"card_version\":\"error\",\"url\":\"xxx\",\"error\":\"test error\\n\",\"error_code\":100}";
  static final String JS_ERROR =
      "{\"sdk\":\"1.0.0\",\"level\":\"error\",\"url\":\"xxx\",\"error\":\"{\\\"sentry\\\":{\\\"exception\\\":{\\\"values\\\":[]},\\\"level\\\":\\\"error\\\",\\\"sdk\\\":{\\\"name\\\":\\\"sentry.javascript.lynx\\\",\\\"packages\\\":[{\\\"name\\\":\\\"\\\",\\\"version\\\":\\\"1.0.0\\\"}],\\\"version\\\":\\\"1.0.0\\\",\\\"integrations\\\":[]},\\\"platform\\\":\\\"javascript\\\",\\\"tags\\\":{\\\"lib_version\\\":\\\"\\\",\\\"version_code\\\":\\\"unknown_version\\\",\\\"run_type\\\":\\\"lynx_core\\\",\\\"extra\\\":\\\"at Card onLoad \\\\ntest error\\\",\\\"error_type\\\":\\\"USER_RUNTIME_ERROR\\\"}},\\\"pid\\\":\\\"USER_ERROR\\\",\\\"url\\\":\\\"file://lynx_core.js\\\",\\\"rawError\\\":{\\\"stack\\\":\\\"\\\",\\\"message\\\":\\\"at Card onLoad \\\\ntest error\\\"}}\",\"error_code\":201}";
  static final String JS_ERROR_MISSING_MESSAGE_FIELD =
      "{\"sdk\":\"1.0.0\",\"level\":\"error\",\"url\":\"xxx\",\"error\":\"{\\\"sentry\\\":{\\\"exception\\\":{\\\"values\\\":[]},\\\"level\\\":\\\"error\\\",\\\"sdk\\\":{\\\"name\\\":\\\"sentry.javascript.lynx\\\",\\\"packages\\\":[{\\\"name\\\":\\\"\\\",\\\"version\\\":\\\"1.0.0\\\"}],\\\"version\\\":\\\"1.0.0\\\",\\\"integrations\\\":[]},\\\"platform\\\":\\\"javascript\\\",\\\"tags\\\":{\\\"lib_version\\\":\\\"\\\",\\\"version_code\\\":\\\"unknown_version\\\",\\\"run_type\\\":\\\"lynx_core\\\",\\\"extra\\\":\\\"at Card onLoad \\\\ntest error\\\",\\\"error_type\\\":\\\"USER_RUNTIME_ERROR\\\"}},\\\"pid\\\":\\\"USER_ERROR\\\",\\\"url\\\":\\\"file://lynx_core.js\\\",\\\"rawError\\\":{\\\"stack\\\":\\\"\\\"}}\",\"error_code\":201}";
  static final String RAW_MESSAGE = "raw error message\n";

  // expected messages
  static final String EXPECT_BRIEF_NATIVE_ERROR_MSG = "test error ";
  static final String EXPECT_BRIEF_JS_ERROR_MSG = "at Card onLoad  test error";
  static final String EXPECT_BRIEF_JS_ERROR_MSG_MISSING_MESSAGE_FIELD =
      "{\"sdk\":\"1.0.0\",\"level\":\"error\",\"url\":\"xxx\",\"error\"";
  static final String EXPECT_BRIEF_RAW_MESSAGE = "raw error message ";

  @Test
  public void extractBriefMessage() {
    // test extract brief message from native error message
    String briefNativeErrorMsg = LynxLogBoxManager.extractBriefMessage(NATIVE_ERROR);
    assertTrue(briefNativeErrorMsg.equals(EXPECT_BRIEF_NATIVE_ERROR_MSG));

    // test extract brief message from js error message
    String briefJSErrorMsg = LynxLogBoxManager.extractBriefMessage(JS_ERROR);
    assertTrue(briefJSErrorMsg.equals(EXPECT_BRIEF_JS_ERROR_MSG));

    // test extract brief message from js error message that missing message field
    String briefJSErrorMsgMissingMsg =
        LynxLogBoxManager.extractBriefMessage(JS_ERROR_MISSING_MESSAGE_FIELD);
    assertTrue(briefJSErrorMsgMissingMsg.equals(EXPECT_BRIEF_JS_ERROR_MSG_MISSING_MESSAGE_FIELD));

    // test extract brief message from raw error message
    String briefRawMessage = LynxLogBoxManager.extractBriefMessage(RAW_MESSAGE);
    assertTrue(briefRawMessage.equals(EXPECT_BRIEF_RAW_MESSAGE));

    // test pass empty string to method
    String emptyMsg = LynxLogBoxManager.extractBriefMessage("");
    assertTrue(emptyMsg.isEmpty());
  }
}
