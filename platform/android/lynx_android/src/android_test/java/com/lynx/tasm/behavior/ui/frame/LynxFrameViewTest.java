// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.frame;

import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.content.Context;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.DynamicFromMap;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxTemplateRender;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.LynxViewClientV2.LynxResourceLoadInfo;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.base.LynxConsumer;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxCustomEvent;
import com.lynx.tasm.image.LynxImageConfig;
import com.lynx.tasm.image.LynxImageRequestDelegate;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import com.lynx.tasm.resourceprovider.LynxResourceRequest.LynxResourceType;
import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Map;
import junit.framework.TestCase;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;

@RunWith(AndroidJUnit4.class)
public class LynxFrameViewTest extends TestCase {
  private static final String EVENT_LAYOUT_CHANGE = "layoutchange";

  @BeforeClass
  public static void initializeLynx() {
    Application application = ApplicationProvider.getApplicationContext();
    LynxEnv.inst().init(application, null, null, null, null);
  }

  @Test
  public void childBuilderPreservesImageMonitoringConfiguration() throws Exception {
    LynxImageConfig config = new LynxImageConfig();
    LynxImageRequestDelegate delegate = mock(LynxImageRequestDelegate.class);
    config.setImageRequestDelegate(delegate);
    config.setEnableImageLoadCallback(true);
    Map<String, String> params = new HashMap<>();
    params.put("scene", "frame-image");
    LynxViewBuilder builder =
        new LynxViewBuilder().setLynxImageConfig(config).setImageCustomParam(params);
    LynxTemplateRender render =
        mock(LynxTemplateRender.class, org.mockito.Mockito.CALLS_REAL_METHODS);
    Field builderField = LynxTemplateRender.class.getDeclaredField("mLynxViewBuilder");
    builderField.setAccessible(true);
    builderField.set(render, builder);

    LynxViewBuilder child = render.getLynxViewBuilder();
    assertSame(config, child.getLynxImageConfig());
    assertSame(delegate, child.getLynxImageConfig().getImageRequestDelegate());
    assertTrue(child.getLynxImageConfig().getEnableImageLoadCallback());
    Field paramsField = LynxViewBuilder.class.getDeclaredField("mImageCustomParam");
    paramsField.setAccessible(true);
    assertSame(params, paramsField.get(child));
    builderField.set(render, child);
    assertSame(config, render.getLynxViewBuilder().getLynxImageConfig());
  }

  @Test
  public void nestedFramesForwardImageResultsOnceAndStopAfterDestroy() {
    Context androidContext = ApplicationProvider.getApplicationContext();
    LynxContext rootContext = mock(LynxContext.class);
    LynxFrameView parent = new LynxFrameView(androidContext);
    parent.init(rootContext);
    FramePerformanceClient parentClient = new FramePerformanceClient(parent);
    LynxContext parentContext = mock(LynxContext.class);
    doAnswer(invocation -> {
      parentClient.onResourceLoaded(invocation.getArgument(0));
      return null;
    })
        .when(parentContext)
        .notifyResourceLoaded(org.mockito.ArgumentMatchers.any());
    LynxFrameView child = new LynxFrameView(androidContext);
    child.init(parentContext);
    FramePerformanceClient childClient = new FramePerformanceClient(child);
    LynxResourceLoadInfo success =
        new LynxResourceLoadInfo(LynxResourceType.LynxResourceTypeImage, 0, null);
    LynxResourceLoadInfo failure =
        new LynxResourceLoadInfo(LynxResourceType.LynxResourceTypeImage, 7, "image failed");
    LynxResourceLoadInfo font =
        new LynxResourceLoadInfo(LynxResourceType.LynxResourceTypeFont, 0, null);

    childClient.onResourceLoaded(success);
    childClient.onResourceLoaded(failure);
    childClient.onResourceLoaded(font);
    child.destroy();
    childClient.onResourceLoaded(success);

    verify(rootContext, times(1)).notifyResourceLoaded(success);
    verify(rootContext, times(1)).notifyResourceLoaded(failure);
    verify(rootContext, never()).notifyResourceLoaded(font);
  }

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

  private TemplateData getPendingInitData(LynxFrameView frameView) throws Exception {
    Field field = LynxFrameView.class.getDeclaredField("mInitData");
    field.setAccessible(true);
    return (TemplateData) field.get(frameView);
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

  @Test
  public void setDataMapStoresPendingTemplateData() throws Exception {
    LynxUIOwner uiOwner = mock(LynxUIOwner.class);
    EventEmitter eventEmitter = mock(EventEmitter.class);
    UIFrame uiFrame = createFrameUI(eventEmitter, uiOwner);

    JavaOnlyMap data = new JavaOnlyMap();
    data.putInt("value", 2);
    JavaOnlyMap props = new JavaOnlyMap();
    props.putMap("data", data);

    uiFrame.setData(new DynamicFromMap(props, "data"));

    TemplateData pendingData = getPendingInitData(uiFrame.getView());
    assertNotNull(pendingData);
    Map<Object, Object> pendingMap = pendingData.toMap();
    assertEquals(2.0, ((Number) pendingMap.get("value")).doubleValue(), 0.0);
    pendingData.recycle();
  }
}
