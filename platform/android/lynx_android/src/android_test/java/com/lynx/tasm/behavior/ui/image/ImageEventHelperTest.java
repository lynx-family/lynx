// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import com.lynx.tasm.LynxView;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.image.ImageEventHelper;
import java.util.HashMap;
import java.util.Map;
import org.json.JSONObject;
import org.junit.Test;

public class ImageEventHelperTest {
  @Test
  public void getReportDataWithImageInfo() throws Exception {
    LynxContext context = mock(LynxContext.class);
    LynxView lynxView = mock(LynxView.class);
    when(context.getLynxView()).thenReturn(lynxView);
    when(lynxView.getTemplateUrl()).thenReturn("https://example.com/template.js");

    JSONObject info = new JSONObject();
    info.put("viewWidth", 100);
    info.put("viewHeight", 200);
    info.put("width", 300);
    info.put("height", 400);
    info.put("isFlattenAnim", 1);
    info.put("config", "origin");
    JSONObject data = ImageEventHelper.getReportData(
        context, "https://example.com/image.png", true, true, 1000, 250, 500, 1500, 4096, info);

    assertNotNull(data);
    JSONObject timeMetrics = data.getJSONObject("timeMetrics");
    assertEquals(0.25, timeMetrics.getDouble("fetchTime"), 0.0001);
    assertEquals(0.5, timeMetrics.getDouble("completeTime"), 0.0001);
    assertEquals(1000, timeMetrics.getLong("fetchTimeStamp"));
    assertEquals(1500, timeMetrics.getLong("finishTimeStamp"));

    JSONObject metric = data.getJSONObject("metric");
    assertEquals("https://example.com/template.js", metric.getString("url"));
    assertEquals(100, metric.getLong("viewWidth"));
    assertEquals(200, metric.getLong("viewHeight"));
    assertEquals(300, metric.getLong("width"));
    assertEquals(400, metric.getLong("height"));
    assertEquals(1, metric.getInt("flattenAnim"));
    assertEquals("origin", metric.getString("config"));

    assertEquals("https://example.com/image.png", data.getString("image_url"));
    assertEquals(1, data.getInt("successRate"));
    assertEquals(4096.0, data.getDouble("memoryCost"), 0.0001);
    assertEquals(1, data.getInt("resourceFromMemoryCache"));
  }

  @Test
  public void getReportDataNormalizesEmptyTemplateAndInvalidImageInfo() throws Exception {
    LynxContext context = mock(LynxContext.class);
    LynxView lynxView = mock(LynxView.class);
    when(context.getLynxView()).thenReturn(lynxView);
    when(lynxView.getTemplateUrl()).thenReturn("");

    JSONObject info = new JSONObject();
    info.put("viewWidth", 0);
    info.put("viewHeight", -1);
    info.put("width", 0);
    info.put("height", -2);

    JSONObject data = ImageEventHelper.getReportData(
        context, "res://image", false, false, 10, 20, 30, 40, 0, info);

    assertNotNull(data);
    JSONObject metric = data.getJSONObject("metric");
    assertEquals("", metric.getString("url"));
    assertEquals(-1, metric.getLong("viewWidth"));
    assertEquals(-1, metric.getLong("viewHeight"));
    assertEquals(-1, metric.getLong("width"));
    assertEquals(-1, metric.getLong("height"));
    assertEquals(0, data.getInt("successRate"));
    assertEquals(0, data.getInt("resourceFromMemoryCache"));
  }

  @Test
  public void getReportDataWithoutContextSkipsTemplateMetric() throws Exception {
    JSONObject data =
        ImageEventHelper.getReportData(null, "res://image", true, false, 10, 20, 30, 40, 128, null);

    assertNotNull(data);
    assertFalse(data.has("metric"));
    assertEquals("res://image", data.getString("image_url"));
    assertEquals(1, data.getInt("successRate"));
    assertEquals(128.0, data.getDouble("memoryCost"), 0.0001);
  }

  @Test
  public void reportImageEventAcceptsCustomParamsAndNullContext() {
    LynxContext context = mock(LynxContext.class);
    when(context.getInstanceId()).thenReturn(7);
    Map<String, String> customParams = new HashMap<>();
    customParams.put("biz", "image_test");

    ImageEventHelper.reportImageEvent(
        context, "res://image", 0, true, 2, 100, 150, true, 320, 240, customParams);
    ImageEventHelper.reportImageEvent(
        null, "res://image", -1, false, 0, 100, 150, false, 0, 0, null);

    assertTrue(customParams.containsKey("biz"));
  }

  @Test
  public void monitorReporterSkipsInvalidInput() {
    ImageEventHelper.monitorReporterV2(null, -1, "", true, false, 100, 200, 1, null);
    ImageEventHelper.monitorReporterV2(null, -1, "res://image", true, false, 0, 200, 1, null);
    ImageEventHelper.monitorReporter(null, "", true, false, 100, 200, 1, null);
    ImageEventHelper.monitorReporter(null, "res://image", true, false, 100, 0, 1, null);
  }

  @Test
  public void asyncReportersAcceptValidInput() throws Exception {
    LynxContext context = mock(LynxContext.class);
    LynxView lynxView = mock(LynxView.class);
    when(context.getLynxView()).thenReturn(lynxView);
    when(context.getTemplateUrl()).thenReturn("https://example.com/template.js");
    when(lynxView.getRenderPhase()).thenReturn("load");

    JSONObject info = new JSONObject();
    info.put("viewWidth", 100);
    info.put("viewHeight", 200);
    info.put("width", 300);
    info.put("height", 400);
    info.put("isFlattenAnim", 1);
    info.put("config", "ARGB_8888");

    ImageEventHelper.monitorReporterV2(
        context, 1, "res://image", true, false, 1000, 1600, 4096, info);
    ImageEventHelper.monitorReporter(null, "res://image", false, true, 1000, 1800, 2048, null);
    ImageEventHelper.reportImageInfo(context, "res://image", true, true, 1000, 1800, 2048, 0);

    Thread.sleep(100);
  }
}
