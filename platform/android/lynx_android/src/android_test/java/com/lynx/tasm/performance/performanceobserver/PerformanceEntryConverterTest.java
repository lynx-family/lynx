// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.performanceobserver;

import static org.junit.Assert.*;

import com.lynx.react.bridge.ReadableMap;
import java.util.HashMap;
import org.junit.Test;
import org.mockito.Mockito;

public class PerformanceEntryConverterTest {
  @Test
  public void testPipelineEntryCreation() {
    verifyEntryCreation("pipeline", "updateTriggeredByBts", PipelineEntry.class);
    verifyEntryCreation("pipeline", "reactLynxHydrate", PipelineEntry.class);
  }

  @Test
  public void testLoadBundleEntry() {
    verifyEntryCreation("pipeline", "loadBundle", LoadBundleEntry.class);
  }

  @Test
  public void testInitEntries() {
    verifyEntryCreation("init", "container", InitContainerEntry.class);
    verifyEntryCreation("init", "lynxview", InitLynxviewEntry.class);
  }

  @Test
  public void testMetricEntries() {
    verifyEntryCreation("metric", "fcp", MetricFcpEntry.class);
    verifyEntryCreation("metric", "fsp", MetricFspEntry.class);
    verifyEntryCreation("metric", "actualFmp", MetricActualFmpEntry.class);
  }

  @Test
  public void testDefaultEntry() {
    ReadableMap map = createMockMap("unknownType", "anyName");
    assertTrue(PerformanceEntryConverter.makePerformanceEntry(map) instanceof PerformanceEntry);
  }

  private <T extends PerformanceEntry> void verifyEntryCreation(
      String type, String name, Class<T> expectedClass) {
    ReadableMap map = createMockMap(type, name);
    PerformanceEntry entry = PerformanceEntryConverter.makePerformanceEntry(map);
    assertTrue(expectedClass.isInstance(entry));
    assertEquals(name, entry.name);
  }

  private ReadableMap createMockMap(String type, String name) {
    ReadableMap map = Mockito.mock(ReadableMap.class);
    Mockito.when(map.getString("entryType")).thenReturn(type);
    Mockito.when(map.getString("name")).thenReturn(name);
    HashMap<String, Object> props = new HashMap<>();
    props.put("entryType", type);
    props.put("name", name);
    Mockito.when(map.asHashMap()).thenReturn(props);
    return map;
  }
}
