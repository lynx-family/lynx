// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyArray;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.util.List;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;

@RunWith(AndroidJUnit4.class)
public class LynxHostScriptViewCommandsTest {
  private static final long VIEW_ID = 123456789;
  private static final String URL = "assets://command.js";
  private LynxView mView;
  private LynxTemplateRender mRender;
  private boolean mHostScriptInstalled;

  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(
        () -> mHostScriptInstalled = isHostScriptAvailable());
    mView = mock(LynxView.class);
    mRender = mock(LynxTemplateRender.class);
    when(mRender.getLogContextSnapshot()).thenReturn(new LynxLogContext(VIEW_ID, 1, 2));
  }

  @Test
  public void loadTemplatePreservesBinaryDataAndProcessingState() {
    onUiThread(() -> {
      byte[] bytes = new byte[] {0, (byte) 0xff, (byte) 0x80};
      loadTemplate(bytes, "{\"value\":\"hello \\ud83d\\ude00\"}", "{\"theme\":\"dark\"}",
          "processData", true);

      ArgumentCaptor<LynxLoadMeta> request = ArgumentCaptor.forClass(LynxLoadMeta.class);
      verify(mView).loadTemplate(request.capture());
      LynxLoadMeta meta = request.getValue();
      assertEquals(URL, meta.getUrl());
      assertSame(bytes, meta.getTemplateBinary());
      assertEquals("hello \uD83D\uDE00", meta.getInitialData().toMap().get("value"));
      assertEquals("processData", meta.getInitialData().processorName());
      assertTrue(meta.getInitialData().isReadOnly());
      assertEquals("dark", meta.getGlobalProps().toMap().get("theme"));
      assertNull(meta.getGlobalProps().processorName());
      assertFalse(meta.getGlobalProps().isReadOnly());
    });
  }

  @Test
  public void urlLoadsKeepMissingAndEmptyFragmentsAbsent() {
    onUiThread(() -> {
      loadTemplate(null, null, null, null, false);
      loadTemplate(new byte[0], "", "", "", true);

      ArgumentCaptor<LynxLoadMeta> requests = ArgumentCaptor.forClass(LynxLoadMeta.class);
      verify(mView, times(2)).loadTemplate(requests.capture());
      for (LynxLoadMeta meta : requests.getAllValues()) {
        assertEquals(URL, meta.getUrl());
        assertNull(meta.getTemplateBinary());
        assertNull(meta.getInitialData());
        assertNull(meta.getGlobalProps());
      }
    });
  }

  @Test
  public void loadTemplateKeepsPlainDataMutableWithoutAProcessor() {
    onUiThread(() -> {
      for (String processor : new String[] {null, ""}) {
        loadTemplate(null, "{\"value\":1}", null, processor, false);
      }
      ArgumentCaptor<LynxLoadMeta> requests = ArgumentCaptor.forClass(LynxLoadMeta.class);
      verify(mView, times(2)).loadTemplate(requests.capture());
      for (LynxLoadMeta meta : requests.getAllValues()) {
        assertEquals(1, ((Number) meta.getInitialData().toMap().get("value")).intValue());
        assertNull(meta.getInitialData().processorName());
        assertFalse(meta.getInitialData().isReadOnly());
      }
    });
  }

  @Test
  public void ssrCommandsSelectRenderOrHydrateAndPreserveData() {
    onUiThread(() -> {
      byte[] bytes = new byte[] {1, 2, 3};
      loadSSR(bytes, "{\"value\":\"render\"}", false);
      loadSSR(bytes, "{\"value\":\"hydrate\"}", true);

      ArgumentCaptor<TemplateData> data = ArgumentCaptor.forClass(TemplateData.class);
      verify(mView).renderSSR(eq(bytes), eq(URL), data.capture());
      assertEquals("render", data.getValue().toMap().get("value"));
      assertFalse(data.getValue().isReadOnly());
      assertNull(data.getValue().processorName());
      verify(mView).ssrHydrate(eq(bytes), eq(URL), data.capture());
      assertEquals("hydrate", data.getValue().toMap().get("value"));
    });
  }

  @Test
  public void ssrCommandsKeepMissingAndEmptyDataAbsent() {
    onUiThread(() -> {
      byte[] bytes = new byte[] {1};
      loadSSR(bytes, null, false);
      loadSSR(bytes, "", true);
      verify(mView).renderSSR(bytes, URL, (TemplateData) null);
      verify(mView).ssrHydrate(bytes, URL, (TemplateData) null);
    });
  }

  @Test
  public void metadataCommandsPreserveIndependentOptionalFragments() {
    onUiThread(() -> {
      updateMetaData("{\"value\":1}", "{\"theme\":\"dark\"}");
      updateMetaData("{\"value\":2}", null);
      updateMetaData(null, "{\"theme\":\"light\"}");
      updateMetaData(null, null);

      ArgumentCaptor<LynxUpdateMeta> requests = ArgumentCaptor.forClass(LynxUpdateMeta.class);
      verify(mView, times(4)).updateMetaData(requests.capture());
      List<LynxUpdateMeta> updates = requests.getAllValues();
      assertEquals(1, ((Number) updates.get(0).getUpdatedData().toMap().get("value")).intValue());
      assertEquals("dark", updates.get(0).getUpdatedGlobalProps().toMap().get("theme"));
      assertEquals(2, ((Number) updates.get(1).getUpdatedData().toMap().get("value")).intValue());
      assertNull(updates.get(1).getUpdatedGlobalProps());
      assertNull(updates.get(2).getUpdatedData());
      assertEquals("light", updates.get(2).getUpdatedGlobalProps().toMap().get("theme"));
      assertNull(updates.get(3).getUpdatedData());
      assertNull(updates.get(3).getUpdatedGlobalProps());
    });
  }

  @Test
  public void reloadTemplatePreservesDataAndGlobalProps() {
    onUiThread(() -> {
      reloadTemplate("{\"value\":1}", "{\"theme\":\"dark\"}");
      ArgumentCaptor<TemplateData> data = ArgumentCaptor.forClass(TemplateData.class);
      ArgumentCaptor<TemplateData> props = ArgumentCaptor.forClass(TemplateData.class);
      verify(mView).reloadTemplate(data.capture(), props.capture());
      assertEquals(1, ((Number) data.getValue().toMap().get("value")).intValue());
      assertEquals("dark", props.getValue().toMap().get("theme"));
    });
  }

  @Test
  public void reloadTemplateSuppliesEmptyDataWhenOmitted() {
    onUiThread(() -> {
      reloadTemplate(null, null);
      reloadTemplate("", "");
      ArgumentCaptor<TemplateData> data = ArgumentCaptor.forClass(TemplateData.class);
      verify(mView, times(2)).reloadTemplate(data.capture(), eq(null));
      for (TemplateData value : data.getAllValues()) {
        assertTrue(value.isEmpty());
      }
    });
  }

  @Test
  public void globalEventConvertsNestedParamsAndUnicode() {
    onUiThread(() -> {
      sendGlobalEvent("[1,true,null,{\"value\":\"hello \\ud83d\\ude00\","
          + "\"empty\":null,\"items\":[false,{\"n\":1}]},[2]]");
      ArgumentCaptor<JavaOnlyArray> params = ArgumentCaptor.forClass(JavaOnlyArray.class);
      verify(mView).sendGlobalEvent(eq("event"), params.capture());
      JavaOnlyArray values = params.getValue();
      assertEquals(5, values.size());
      assertEquals(1, values.getInt(0));
      assertTrue(values.getBoolean(1));
      assertTrue(values.isNull(2));
      assertEquals("hello \uD83D\uDE00", values.getMap(3).getString("value"));
      assertTrue(values.getMap(3).isNull("empty"));
      assertFalse(values.getMap(3).getArray("items").getBoolean(0));
      assertEquals(1, values.getMap(3).getArray("items").getMap(1).getInt("n"));
      assertEquals(2, values.getArray(4).getInt(0));
    });
  }

  @Test
  public void globalEventAllowsEmptyParams() {
    onUiThread(() -> {
      sendGlobalEvent("[]");
      ArgumentCaptor<JavaOnlyArray> params = ArgumentCaptor.forClass(JavaOnlyArray.class);
      verify(mView).sendGlobalEvent(eq("event"), params.capture());
      assertEquals(0, params.getValue().size());
    });
  }

  @Test
  public void registeredViewForwardsLifecycleAndErrorDetails() {
    Assume.assumeTrue(mHostScriptInstalled);
    onUiThread(() -> {
      LynxError error = new LynxError(123, "failure \uD83D\uDE00");
      JSONArray events = captureEvents(() -> {
        ArgumentCaptor<LynxViewClient> client = ArgumentCaptor.forClass(LynxViewClient.class);
        verify(mView).addLynxViewClient(client.capture());
        client.getValue().onLoadSuccess();
        client.getValue().onFirstScreen();
        client.getValue().onPageUpdate();
        client.getValue().onDataUpdated();
        client.getValue().onUpdateDataWithoutChange();
        client.getValue().onReceivedError((LynxError) null);
        client.getValue().onReceivedError(error);
      });
      assertEquals(6, events.length());
      String[] names = {
          "loadSuccess", "firstScreen", "pageUpdate", "dataUpdated", "dataUpdated", "error"};
      for (int i = 0; i < names.length; i++) {
        JSONObject event = eventAt(events, i);
        assertEquals(VIEW_ID, event.optLong("viewId"));
        assertEquals(names[i], event.optString("name"));
        assertEquals(i == 5 ? error.getErrorCode() : 0, event.optInt("code"));
        assertEquals(i == 5 ? error.getSummaryMessage() : "", event.optString("message"));
      }
    });
  }

  @Test
  public void registrationSkipsUnavailableRegistryAndViewIdentity() {
    onUiThread(() -> {
      try {
        Field available = LynxHostScriptViewHooks.class.getDeclaredField("sRegistryAvailable");
        available.setAccessible(true);
        boolean original = available.getBoolean(null);
        try {
          available.setBoolean(null, false);
          LynxHostScriptViewHooks.register(mView, mRender);
          LynxHostScriptViewHooks.unregister(mRender);
          available.setBoolean(null, true);
          LynxHostScriptViewHooks.register(mView, null);
          LynxHostScriptViewHooks.unregister(null);
          when(mRender.getLogContextSnapshot()).thenReturn(LynxLogContext.UNAVAILABLE);
          LynxHostScriptViewHooks.register(mView, mRender);
          LynxHostScriptViewHooks.unregister(mRender);
          verify(mView, never()).addLynxViewClient(any());
        } finally {
          available.setBoolean(null, original);
        }
      } catch (ReflectiveOperationException error) {
        throw new AssertionError(error);
      }
    });
  }

  @Test
  public void malformedGlobalEventReportsAnErrorWithoutDispatching() {
    Assume.assumeTrue(mHostScriptInstalled);
    onUiThread(() -> {
      JSONArray events = captureEvents(() -> sendGlobalEvent("{\"invalid\":true}"));
      verify(mView, never()).sendGlobalEvent(any(), any());
      assertEquals(1, events.length());
      JSONObject error = eventAt(events, 0);
      assertEquals("error", error.optString("name"));
      assertEquals(-1, error.optInt("code"));
      assertTrue(error.optString("message").contains("IllegalArgumentException"));
      sendGlobalEvent("[]");
      verify(mView).sendGlobalEvent(eq("event"), any());
    });
  }

  @Test
  public void platformCommandFailureIsReportedWithoutEscaping() {
    Assume.assumeTrue(mHostScriptInstalled);
    onUiThread(() -> {
      doThrow(new IllegalStateException("platform failure \uD83D\uDE00"))
          .when(mView)
          .loadTemplate(any());
      JSONArray events = captureEvents(() -> loadTemplate(null, null, null, null, false));
      verify(mView).loadTemplate(any());
      assertEquals(1, events.length());
      JSONObject error = eventAt(events, 0);
      assertEquals("error", error.optString("name"));
      assertEquals(-1, error.optInt("code"));
      assertEquals("java.lang.IllegalStateException: platform failure \uD83D\uDE00",
          error.optString("message"));
    });
  }

  private void loadTemplate(
      byte[] bytes, String data, String props, String processor, boolean readOnly) {
    invokeCommand("loadTemplate",
        new Class<?>[] {LynxView.class, long.class, byte[].class, String.class, String.class,
            String.class, String.class, boolean.class},
        mView, VIEW_ID, bytes, URL, data, props, processor, readOnly);
  }

  private void loadSSR(byte[] bytes, String data, boolean hydrate) {
    invokeCommand("loadSSR",
        new Class<?>[] {
            LynxView.class, long.class, byte[].class, String.class, String.class, boolean.class},
        mView, VIEW_ID, bytes, URL, data, hydrate);
  }

  private void updateMetaData(String data, String props) {
    invokeCommand("updateMetaData",
        new Class<?>[] {LynxView.class, long.class, String.class, String.class}, mView, VIEW_ID,
        data, props);
  }

  private void reloadTemplate(String data, String props) {
    invokeCommand("reloadTemplate",
        new Class<?>[] {LynxView.class, long.class, String.class, String.class}, mView, VIEW_ID,
        data, props);
  }

  private void sendGlobalEvent(String params) {
    invokeCommand("sendGlobalEvent",
        new Class<?>[] {LynxView.class, long.class, String.class, String.class}, mView, VIEW_ID,
        "event", params);
  }

  private JSONArray captureEvents(Runnable command) {
    byte[] events = nativeCaptureEvents(VIEW_ID, () -> {
      LynxHostScriptViewHooks.register(mView, mRender);
      try {
        command.run();
      } finally {
        LynxHostScriptViewHooks.unregister(mRender);
      }
    });
    try {
      return new JSONArray(new String(events, StandardCharsets.UTF_8));
    } catch (JSONException error) {
      throw new AssertionError(error);
    }
  }

  private static JSONObject eventAt(JSONArray events, int index) {
    try {
      return events.getJSONObject(index);
    } catch (JSONException error) {
      throw new AssertionError(error);
    }
  }

  private static void invokeCommand(String name, Class<?>[] types, Object... args) {
    try {
      Method method = LynxHostScriptViewHooks.class.getDeclaredMethod(name, types);
      method.setAccessible(true);
      method.invoke(null, args);
    } catch (ReflectiveOperationException error) {
      throw new AssertionError(error);
    }
  }

  private static void onUiThread(Runnable command) {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(command);
  }

  private static boolean isHostScriptAvailable() {
    try {
      Field available = LynxHostScriptViewHooks.class.getDeclaredField("sRegistryAvailable");
      available.setAccessible(true);
      return available.getBoolean(null);
    } catch (ReflectiveOperationException error) {
      throw new AssertionError(error);
    }
  }

  private static native byte[] nativeCaptureEvents(long viewId, Runnable command);
}
