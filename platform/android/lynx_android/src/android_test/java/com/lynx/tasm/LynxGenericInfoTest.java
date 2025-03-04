// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.*;

import com.lynx.react.bridge.JavaOnlyMap;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Test;

public class LynxGenericInfoTest {
  @Test
  public void toJSONObject() {
    LynxGenericInfo lynxGenericInfo = new LynxGenericInfo(null);
    JSONObject data = lynxGenericInfo.toJSONObject();
    assertTrue(data.has("thread_mode"));
    try {
      assertEquals(data.getInt("thread_mode"), -1);
    } catch (JSONException e) {
      fail();
    }
    assertTrue(data.has("lynx_sdk_version"));
    assertFalse(data.has("url"));
    assertFalse(data.has("relative_path"));
    assertFalse(data.has("lynx_target_sdk_version"));
    assertFalse(data.has("lynx_dsl"));
    assertFalse(data.has("lynx_lepus_type"));
    assertFalse(data.has("lynx_page_version"));
  }

  @Test
  public void updatePageConfigInfo() {
    LynxGenericInfo lynxGenericInfo = new LynxGenericInfo(null);
    JavaOnlyMap map = new JavaOnlyMap();
    map.putString("pageType", "tt");
    map.putBoolean("enableLepusNG", true);
    map.putString("pageVersion", "1.0.0");
    map.putString("targetSdkVersion", "2.9");
    lynxGenericInfo.updatePageConfigInfo(new PageConfig(map));
    JSONObject data = lynxGenericInfo.toJSONObject();
    assertTrue(data.has("thread_mode"));
    assertTrue(data.has("lynx_sdk_version"));
    assertFalse(data.has("url"));
    assertFalse(data.has("relative_path"));
    try {
      assertEquals(data.getString("lynx_target_sdk_version"), "2.9");
      assertEquals(data.getString("lynx_dsl"), "ttml");
      assertEquals(data.getString("lynx_lepus_type"), "lepusNG");
      assertEquals(data.getString("lynx_page_version"), "1.0.0");
    } catch (JSONException e) {
      fail();
    }
  }

  @Test
  public void updateLynxUrl() {
    String[] originalUrls = new String[] {// absolute urls
        "https://obj/template.js?channel=test_channel&abkeys=s_optimize",
        "https://obj/template.js?bundle=test_channel&abkeys=s_optimize",
        "https://obj/template.js?url=test_channel&abkeys=s_optimize",
        "https://obj/template.js?surl=test_channel&abkeys=s_optimize",
        // relative urls
        "this is a lynx url"};

    String[] expectedRelativeUrls = new String[] {"https://obj/template.js?channel=test_channel",
        "https://obj/template.js?bundle=test_channel", "https://obj/template.js?url=test_channel",
        "https://obj/template.js?surl=test_channel", "this is a lynx url"};

    for (int caseIdx = 0; caseIdx < originalUrls.length; caseIdx++) {
      LynxGenericInfo lynxGenericInfo = new LynxGenericInfo(null);
      lynxGenericInfo.updateLynxUrl(null, originalUrls[caseIdx]);
      JSONObject data = lynxGenericInfo.toJSONObject();
      assertTrue(data.has("thread_mode"));
      assertTrue(data.has("lynx_sdk_version"));
      try {
        assertEquals(data.getString("url"), originalUrls[caseIdx]);
        assertEquals(data.getString("relative_path"), expectedRelativeUrls[caseIdx]);
      } catch (JSONException e) {
        fail();
      }
      assertFalse(data.has("lynx_target_sdk_version"));
      assertFalse(data.has("lynx_dsl"));
      assertFalse(data.has("lynx_lepus_type"));
      assertFalse(data.has("lynx_page_version"));
    }

    // Test null corner case
    LynxGenericInfo lynxGenericInfo = new LynxGenericInfo(null);
    lynxGenericInfo.updateLynxUrl(null, null);
    JSONObject data = lynxGenericInfo.toJSONObject();
    assertTrue(data.has("thread_mode"));
    assertTrue(data.has("lynx_sdk_version"));
    assertFalse(data.has("url"));
    assertFalse(data.has("relative_path"));
    assertFalse(data.has("lynx_target_sdk_version"));
    assertFalse(data.has("lynx_dsl"));
    assertFalse(data.has("lynx_lepus_type"));
    assertFalse(data.has("lynx_page_version"));
  }

  @Test
  public void updateThreadStrategy() {
    LynxGenericInfo lynxGenericInfo = new LynxGenericInfo(null);
    lynxGenericInfo.updateThreadStrategy(ThreadStrategyForRendering.ALL_ON_UI);
    JSONObject data = lynxGenericInfo.toJSONObject();
    try {
      assertEquals(data.getInt("thread_mode"), 0);
    } catch (JSONException e) {
      fail();
    }
    assertTrue(data.has("lynx_sdk_version"));
    assertFalse(data.has("url"));
    assertFalse(data.has("relative_path"));
    assertFalse(data.has("lynx_target_sdk_version"));
    assertFalse(data.has("lynx_dsl"));
    assertFalse(data.has("lynx_lepus_type"));
    assertFalse(data.has("lynx_page_version"));
  }
}
