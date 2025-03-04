// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.*;

import com.lynx.tasm.LynxErrorBehavior;
import com.lynx.tasm.LynxSubErrorCode;
import java.util.HashMap;
import java.util.Map;
import org.json.JSONObject;
import org.junit.Before;
import org.junit.Test;

public class LynxErrorTest {
  private LynxError mError;

  private String mTestMessage = "some error occurred";
  private String mTestSuggestion =
      "An unidentified exception occurred that cannot be attributed to a specific cause. Please investigate further based on the available details.";
  private final int TEST_ERROR_CODE = LynxSubErrorCode.E_RESOURCE_IMAGE_EXCEPTION;

  private final static String TEST_CONTEXT_INFO_KEY1 = "lynx_context_test1";
  private final static String TEST_CONTEXT_INFO_VALUE1 = "test context info1";
  private final static String TEST_CONTEXT_INFO_KEY2 = "lynx_context_test2";
  private final static String TEST_CONTEXT_INFO_VALUE2 = "test context info2";

  @Before
  public void setUp() throws Exception {
    Map<String, Object> customInfo = new HashMap<String, Object>() {
      {
        put("info1", "some info1");
        put("info2", "some info2");
      }
    };
    mError = new LynxError(TEST_ERROR_CODE, mTestMessage, mTestSuggestion, LynxError.LEVEL_ERROR);
  }

  @Test
  public void isValid() {
    // test valid error
    assertTrue(mError.isValid());

    // test invalid error construct with empty string
    LynxError error = new LynxError(TEST_ERROR_CODE, "", "", LynxError.LEVEL_ERROR);
    assertFalse(error.isValid());

    // test invalid error construct with null
    error = new LynxError(TEST_ERROR_CODE, null, "", LynxError.LEVEL_ERROR);
    assertFalse(error.isValid());
  }

  @Test
  public void addCustomInfo() {
    // test add valid info
    mError.addCustomInfo("custom_info1", "some error info");
    // test add invalid info
    mError.addCustomInfo("custom_info2", "");
    mError.addCustomInfo("", "");
    mError.addCustomInfo(null, null);
    String jsonStr = mError.getMsg();

    JSONObject jsonObject = null;
    try {
      jsonObject = new JSONObject(jsonStr);
      assertEquals(jsonObject.getString("custom_info1"), "some error info");
      assertFalse(jsonObject.has("custom_info2"));
    } catch (Throwable t) {
      fail();
    }
  }

  @Test
  public void setUserDefineInfo() {
    JSONObject userObject = new JSONObject();
    try {
      userObject.put("key", "value");
    } catch (Throwable t) {
      fail();
    }

    mError.setUserDefineInfo(userObject);
    String jsonStr = mError.getMsg();
    JSONObject jsonObject = null;
    try {
      jsonObject = new JSONObject(jsonStr);
      assertTrue(jsonObject.has("user_define_info"));
    } catch (Throwable t) {
      fail();
    }
  }

  @Test
  public void getMsg() {
    // test valid error
    String jsonStr = mError.getMsg();
    JSONObject jsonObject = null;
    try {
      jsonObject = new JSONObject(jsonStr);
      assertEquals(jsonObject.getInt("error_code"), LynxErrorBehavior.EB_RESOURCE_IMAGE);
      assertEquals(jsonObject.getString("error"), mTestMessage);
      assertEquals(jsonObject.getString("level"), "error");
      assertEquals(jsonObject.getString("fix_suggestion"), mTestSuggestion);
      assertFalse(jsonObject.has("url"));
      assertFalse(jsonObject.has("card_version"));
      assertFalse(jsonObject.has("error_stack"));
    } catch (Throwable t) {
      fail();
    }

    // test regenerate json string after additional info added
    mError.addCustomInfo(TEST_CONTEXT_INFO_KEY1, TEST_CONTEXT_INFO_VALUE1);
    mError.addCustomInfo(TEST_CONTEXT_INFO_KEY2, TEST_CONTEXT_INFO_VALUE2);
    jsonStr = mError.getMsg();
    try {
      jsonObject = new JSONObject(jsonStr);
      JSONObject contextField = jsonObject.getJSONObject("context");
      assertNotNull(contextField);
      assertEquals(contextField.getString(TEST_CONTEXT_INFO_KEY1), TEST_CONTEXT_INFO_VALUE1);
      assertEquals(contextField.getString(TEST_CONTEXT_INFO_KEY2), TEST_CONTEXT_INFO_VALUE2);
    } catch (Throwable t) {
      fail();
    }

    // test invalid error construct with empty string
    LynxError error = new LynxError(TEST_ERROR_CODE, "", "", LynxError.LEVEL_ERROR);
    jsonStr = error.getMsg();
    assertEquals("", jsonStr);

    // test invalid error construct with null
    error = new LynxError(TEST_ERROR_CODE, null, null, LynxError.LEVEL_ERROR);
    jsonStr = error.getMsg();
    assertEquals("", jsonStr);
  }

  @Test
  public void containsCustomField() {
    boolean res = mError.containsCustomField("custom_info1");
    assertFalse(res);

    mError.addCustomInfo("custom_info1", "some error info");
    res = mError.containsCustomField(null);
    assertFalse(res);

    res = mError.containsCustomField("custom_info2");
    assertFalse(res);

    res = mError.containsCustomField("custom_info1");
    assertTrue(res);
  }

  @Test
  public void testLynxErrorWithSubErrorCode() {
    int code = LynxSubErrorCode.E_RESOURCE_IMAGE_EXCEPTION;
    LynxError error = new LynxError(code, "test android error");
    assertEquals(error.getErrorCode(), code / 100);
    LynxSubErrorCode.MetaData metadata = LynxSubErrorCode.getMetaData(code);
    assertNotNull(metadata);
    assertEquals(metadata.mLevel.value, error.getLevel());
    assertEquals(metadata.mFixSuggestion, error.getFixSuggestion());
  }
}
