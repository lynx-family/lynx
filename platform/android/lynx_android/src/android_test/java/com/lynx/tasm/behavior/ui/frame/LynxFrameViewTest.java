// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import java.util.HashMap;
import java.util.Map;
import junit.framework.TestCase;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;

@RunWith(AndroidJUnit4.class)
public class LynxFrameViewTest extends TestCase {
  @Test
  public void shouldLogFrameLoadMetricsEventReturnsTrueForLoadBundlePipeline() {
    HashMap<String, Object> props = new HashMap<>();
    props.put("entryType", "pipeline");
    props.put("name", "loadBundle");

    assertTrue(FramePerformanceClient.shouldHandlePerformanceEntry(new PerformanceEntry(props)));
  }

  @Test
  public void shouldLogFrameLoadMetricsEventReturnsFalseForNonLoadBundlePipeline() {
    HashMap<String, Object> props = new HashMap<>();
    props.put("entryType", "pipeline");
    props.put("name", "updateTriggeredByNative");

    assertFalse(FramePerformanceClient.shouldHandlePerformanceEntry(new PerformanceEntry(props)));
  }

  @Test
  public void shouldLogFrameLoadMetricsEventReturnsFalseForNonPipelineEntry() {
    HashMap<String, Object> props = new HashMap<>();
    props.put("entryType", "metric");
    props.put("name", "loadBundle");

    assertFalse(FramePerformanceClient.shouldHandlePerformanceEntry(new PerformanceEntry(props)));
  }

  @Test
  public void buildFrameLoadMetricsDetailIncludesUrlModeAndEntry() {
    HashMap<String, Object> props = new HashMap<>();
    props.put("entryType", "pipeline");
    props.put("name", "loadBundle");
    props.put("paintEnd", 42D);

    HashMap<String, Object> detail = LynxFrameView.buildFrameLoadMetricsDetail(
        new PerformanceEntry(props), "lynx://frame", true);

    assertEquals("lynx://frame", detail.get("url"));
    assertEquals("embedded", detail.get("mode"));
    assertEquals(props, detail.get("entry"));
  }

  @Test
  public void dispatchFrameLoadMetricsEventSendsCustomEventWhenBound() {
    Context androidContext = ApplicationProvider.getApplicationContext();
    LynxFrameView frameView = new LynxFrameView(androidContext);
    LynxContext lynxContext = mock(LynxContext.class);
    LynxUIOwner uiOwner = mock(LynxUIOwner.class);
    LynxBaseUI ui = mock(LynxBaseUI.class);
    EventEmitter eventEmitter = mock(EventEmitter.class);

    Map<String, EventsListener> events = new HashMap<>();
    events.put(LynxFrameView.EVENT_LOAD_METRICS,
        new EventsListener(
            LynxFrameView.EVENT_LOAD_METRICS, "bindEvent", "onLoadMetrics", null, null));

    when(lynxContext.getUIBodyView()).thenReturn(null);
    when(lynxContext.getLynxUIOwner()).thenReturn(uiOwner);
    when(lynxContext.getEventEmitter()).thenReturn(eventEmitter);
    when(uiOwner.findLynxUIBySign(100)).thenReturn(ui);
    when(ui.getEvents()).thenReturn(events);

    frameView.init(lynxContext);
    frameView.setSign(100);
    frameView.setUrl("lynx://frame");
    frameView.setEmbeddedMode(EmbeddedMode.EMBEDDED_MODE_BASE);

    HashMap<String, Object> props = new HashMap<>();
    props.put("entryType", "pipeline");
    props.put("name", "loadBundle");
    props.put("paintEnd", 42D);

    frameView.onFrameLoadMetricsEvent(new PerformanceEntry(props));

    ArgumentCaptor<LynxCustomEvent> captor = ArgumentCaptor.forClass(LynxCustomEvent.class);
    verify(eventEmitter).sendCustomEvent(captor.capture());
    LynxCustomEvent event = captor.getValue();
    assertEquals(LynxFrameView.EVENT_LOAD_METRICS, event.getName());
    assertEquals(100, event.getTag());
    assertEquals("detail", event.paramsName());
    assertEquals("lynx://frame", event.eventParams().get("url"));
    assertEquals("embedded", event.eventParams().get("mode"));
    assertEquals(props, event.eventParams().get("entry"));
  }

  @Test
  public void dispatchFrameLoadMetricsEventSkipsWhenUnbound() {
    Context androidContext = ApplicationProvider.getApplicationContext();
    LynxFrameView frameView = new LynxFrameView(androidContext);
    LynxContext lynxContext = mock(LynxContext.class);
    LynxUIOwner uiOwner = mock(LynxUIOwner.class);
    LynxBaseUI ui = mock(LynxBaseUI.class);
    EventEmitter eventEmitter = mock(EventEmitter.class);

    when(lynxContext.getUIBodyView()).thenReturn(null);
    when(lynxContext.getLynxUIOwner()).thenReturn(uiOwner);
    when(lynxContext.getEventEmitter()).thenReturn(eventEmitter);
    when(uiOwner.findLynxUIBySign(100)).thenReturn(ui);
    when(ui.getEvents()).thenReturn(new HashMap<>());

    frameView.init(lynxContext);
    frameView.setSign(100);

    HashMap<String, Object> props = new HashMap<>();
    props.put("entryType", "pipeline");
    props.put("name", "loadBundle");

    frameView.onFrameLoadMetricsEvent(new PerformanceEntry(props));

    verify(eventEmitter, never()).sendCustomEvent(org.mockito.ArgumentMatchers.any());
  }
}
