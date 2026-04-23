// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.base.LynxConsumer;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.UIBody;
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
  private static final String EVENT_LAYOUT_CHANGE = "layoutchange";

  private LynxContext createLynxContext(
      DisplayMetrics screenMetrics, EventEmitter eventEmitter, LynxUIOwner uiOwner) {
    Context androidContext = ApplicationProvider.getApplicationContext();
    LynxContext lynxContext = new LynxContext(androidContext, screenMetrics) {
      @Override
      public void handleException(Exception e) {}

      @Override
      public void findShadowNodeAndRunTask(int sign, LynxConsumer<ShadowNode> task) {}
    };
    UIBody uiBody = mock(UIBody.class);
    when(uiBody.getBodyView()).thenReturn(null);
    lynxContext.setUIBody(uiBody);
    lynxContext.setEventEmitter(eventEmitter);
    lynxContext.setLynxUIOwner(uiOwner);
    return lynxContext;
  }

  private UIFrame createFrameUI(EventEmitter eventEmitter, LynxUIOwner uiOwner) {
    DisplayMetrics screenMetrics = new DisplayMetrics();
    screenMetrics.density = 2.0f;
    LynxContext lynxContext = createLynxContext(screenMetrics, eventEmitter, uiOwner);
    UIFrame uiFrame = new UIFrame(lynxContext);
    uiFrame.setSign(100, "frame");
    uiFrame.getView().setUrl("lynx://frame");
    uiFrame.updateLayout(
        20, 40, 200, 100, 2, 4, 6, 8, 0, 0, 0, 0, 1, 3, 5, 7, new Rect(20, 40, 220, 140));
    when(uiOwner.findLynxUIBySign(100)).thenReturn(uiFrame);
    return uiFrame;
  }

  private Map<String, EventsListener> layoutChangeEvents() {
    Map<String, EventsListener> events = new HashMap<>();
    events.put(EVENT_LAYOUT_CHANGE,
        new EventsListener(EVENT_LAYOUT_CHANGE, "bindEvent", "onLayoutChange", null, null));
    return events;
  }

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
    LynxUIOwner uiOwner = mock(LynxUIOwner.class);
    EventEmitter eventEmitter = mock(EventEmitter.class);
    UIFrame uiFrame = createFrameUI(eventEmitter, uiOwner);

    Map<String, EventsListener> events = new HashMap<>();
    events.put(LynxFrameView.EVENT_LOAD_METRICS,
        new EventsListener(
            LynxFrameView.EVENT_LOAD_METRICS, "bindEvent", "onLoadMetrics", null, null));
    uiFrame.setEvents(events);

    LynxFrameView frameView = uiFrame.getView();
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
    LynxUIOwner uiOwner = mock(LynxUIOwner.class);
    EventEmitter eventEmitter = mock(EventEmitter.class);
    UIFrame uiFrame = createFrameUI(eventEmitter, uiOwner);
    uiFrame.setEvents(new HashMap<>());

    HashMap<String, Object> props = new HashMap<>();
    props.put("entryType", "pipeline");
    props.put("name", "loadBundle");

    uiFrame.getView().onFrameLoadMetricsEvent(new PerformanceEntry(props));

    verify(eventEmitter, never()).sendCustomEvent(org.mockito.ArgumentMatchers.any());
  }

  @Test
  public void setIntrinsicContentSizeDispatchesLayoutChangeEventWhenBound() {
    LynxUIOwner uiOwner = mock(LynxUIOwner.class);
    EventEmitter eventEmitter = mock(EventEmitter.class);
    UIFrame uiFrame = createFrameUI(eventEmitter, uiOwner);
    uiFrame.setEvents(layoutChangeEvents());

    uiFrame.getView().setIntrinsicContentSize(321, 181);

    ArgumentCaptor<LynxCustomEvent> captor = ArgumentCaptor.forClass(LynxCustomEvent.class);
    verify(eventEmitter).sendCustomEvent(captor.capture());
    LynxCustomEvent event = captor.getValue();
    assertEquals(EVENT_LAYOUT_CHANGE, event.getName());
    assertEquals(100, event.getTag());
    assertTrue(event.eventParams().containsKey("id"));
    assertTrue(event.eventParams().containsKey("dataset"));
    assertTrue(event.eventParams().containsKey("left"));
    assertTrue(event.eventParams().containsKey("top"));
    assertTrue(event.eventParams().containsKey("right"));
    assertTrue(event.eventParams().containsKey("bottom"));
    assertEquals(100.0, ((Number) event.eventParams().get("width")).doubleValue(), 0.0);
    assertEquals(50.0, ((Number) event.eventParams().get("height")).doubleValue(), 0.0);
    assertEquals("lynx://frame", event.eventParams().get("frameUrl"));
    assertEquals(160.5, ((Number) event.eventParams().get("intrinsicWidth")).doubleValue(), 0.0);
    assertEquals(90.5, ((Number) event.eventParams().get("intrinsicHeight")).doubleValue(), 0.0);
  }

  @Test
  public void setIntrinsicContentSizeSkipsLayoutChangeEventWhenUnbound() {
    LynxUIOwner uiOwner = mock(LynxUIOwner.class);
    EventEmitter eventEmitter = mock(EventEmitter.class);
    UIFrame uiFrame = createFrameUI(eventEmitter, uiOwner);
    uiFrame.setEvents(new HashMap<>());

    uiFrame.getView().setIntrinsicContentSize(320, 180);

    verify(eventEmitter, never()).sendCustomEvent(org.mockito.ArgumentMatchers.any());
  }

  @Test
  public void setIntrinsicContentSizeDoesNotDispatchDuplicateLayoutChangeEventForSameSize() {
    LynxUIOwner uiOwner = mock(LynxUIOwner.class);
    EventEmitter eventEmitter = mock(EventEmitter.class);
    UIFrame uiFrame = createFrameUI(eventEmitter, uiOwner);
    uiFrame.setEvents(layoutChangeEvents());

    uiFrame.getView().setIntrinsicContentSize(321, 181);
    uiFrame.getView().setIntrinsicContentSize(321, 181);

    verify(eventEmitter, times(1)).sendCustomEvent(org.mockito.ArgumentMatchers.any());
  }
}
