// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax.ui;

import static com.lynx.tasm.base.Assertions.assertNotNull;
import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.lynx.animax.listener.AnimaXErrorParam;
import com.lynx.animax.monitor.AnimaXMonitorUtil;
import com.lynx.animax.monitor.LynxAnimaXMonitorDefault;
import com.lynx.animax.monitor.MetricsAndEventStore;
import com.lynx.tasm.behavior.LynxContext;
import java.util.HashMap;
import java.util.Map;
import org.json.JSONObject;
import org.junit.Test;

public class LynxMonitorTest {
  @Test
  public void testReportError_ContextIsNull() {
    // Given a monitor with a null context
    LynxAnimaXMonitorDefault monitor = new LynxAnimaXMonitorDefault(null);
    monitor.setCurrentUrl("testSrcURL");

    // When reportError is called
    AnimaXErrorParam mockErrorParam = mock(AnimaXErrorParam.class);
    monitor.reportError(mockErrorParam);

    // Then LynxEventReporter.onEvent should not be called
    verify(mockErrorParam, never()).getOriginParams();
  }

  @Test
  public void testReportError_WithValidContextAndErrorInfo() {
    // Given
    LynxContext mockContext = mock(LynxContext.class);
    AnimaXErrorParam mockErrorParam = mock(AnimaXErrorParam.class);
    when(mockErrorParam.getOriginParams()).thenReturn(new HashMap<>());
    when(mockContext.getInstanceId()).thenReturn(1);
    when(mockContext.enableEventReporter()).thenReturn(true);

    LynxAnimaXMonitorDefault monitor = new LynxAnimaXMonitorDefault(mockContext);
    monitor.setCurrentUrl("testSrcURL");

    // When
    monitor.reportError(mockErrorParam);

    // Then
    verify(mockErrorParam, times(1)).getOriginParams();
    verify(mockContext, times(1)).getInstanceId();
  }

  @Test
  public void testReportPerformanceMetrics_WithValidMetricsAndContext() {
    // Given valid metrics and context
    LynxContext mockContext = mock(LynxContext.class);
    MetricsAndEventStore mockMetricsAndEventStore = mock(MetricsAndEventStore.class);
    Map<String, Object> category = new HashMap<>();
    Map<String, Object> metrics = new HashMap<>();
    when(mockMetricsAndEventStore.getCategoryAsMap()).thenReturn(category);
    when(mockMetricsAndEventStore.getMetricsAsMap()).thenReturn(metrics);
    when(mockContext.getInstanceId()).thenReturn(1);
    when(mockContext.getTemplateUrl()).thenReturn("testPageUrl");
    when(mockContext.enableEventReporter()).thenReturn(true);

    LynxAnimaXMonitorDefault monitor = new LynxAnimaXMonitorDefault(mockContext);
    monitor.setCurrentUrl("testSrcURL");

    // When
    monitor.reportPerformanceMetrics(mockMetricsAndEventStore);

    // Then
    verify(mockMetricsAndEventStore, times(1)).getCategoryAsMap();
    verify(mockMetricsAndEventStore, times(1)).getMetricsAsMap();
    verify(mockContext, times(1)).getInstanceId();
    verify(mockContext, times(1)).getTemplateUrl();
  }

  @Test
  public void testConvertHashMapIntoJSON_withEmptyMap() {
    Map<String, Object> emptyMap = new HashMap<>();

    JSONObject result = AnimaXMonitorUtil.convertHashMapIntoJSON(emptyMap);

    assertNotNull(
        result, "convertHashMapIntoJSON should return a non-null JSONObject for an empty map");
    assertEquals(0, result.length());
  }
}
