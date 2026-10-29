// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.app.Application;
import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.util.Collections;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;

@RunWith(AndroidJUnit4.class)
public class LynxHostScriptViewHooksTest {
  private Method mSetEnabled;
  private boolean mWasEnabled;
  private final AtomicInteger mNotifications = new AtomicInteger();

  @Before
  public void setUp() throws Exception {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
    mSetEnabled = LynxHostScriptViewHooks.class.getDeclaredMethod("setEnabled", boolean.class);
    mSetEnabled.setAccessible(true);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mWasEnabled = LynxHostScriptViewHooks.isEnabled();
      setEnabled(false);
    });
  }

  @After
  public void tearDown() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> setEnabled(mWasEnabled));
  }

  @Test
  public void queuedCreationIsDiscardedAfterDisableAndReenable() {
    queueCreation(true, () -> {
      setEnabled(false);
      setEnabled(true);
    });
    assertEquals(0, mNotifications.get());
  }

  @Test
  public void queuedCreationIsDiscardedAfterDisable() {
    queueCreation(true, () -> setEnabled(false));
    assertEquals(0, mNotifications.get());
  }

  @Test
  public void creationWhileDisabledIsNotReplayedAfterEnable() {
    queueCreation(false, () -> setEnabled(true));
    assertEquals(0, mNotifications.get());
  }

  @Test
  public void queuedCreationIsDeliveredWithinEnabledPeriod() {
    queueCreation(true, () -> {});
    assertEquals(1, mNotifications.get());
  }

  @Test
  public void repeatedEnableKeepsQueuedCreation() {
    queueCreation(true, () -> setEnabled(true));
    assertEquals(1, mNotifications.get());
  }

  @Test
  public void newCreationIsDeliveredAfterReenable() {
    queueCreation(true, () -> {
      setEnabled(false);
      setEnabled(true);
    });
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxHostScriptViewHooks.postCreationNotification(mNotifications::incrementAndGet);
      assertEquals(1, mNotifications.get());
    });
    assertEquals(1, mNotifications.get());
  }

  @Test
  public void disabledInterceptorsKeepRequestsWithoutAccessingRender() {
    LynxLoadMeta load = loadMeta();
    TemplateData data = TemplateData.fromString("{\"value\":1}");
    load.initialData = data;
    LynxUpdateMeta update = new LynxUpdateMeta.Builder().setUpdatedData(data).build();

    LynxHostScriptViewHooks.beforeLoadTemplate(null, load);
    assertSame(data, load.initialData);
    assertSame(update, LynxHostScriptViewHooks.beforeUpdateMetaData(null, update));
  }

  @Test
  public void disabledErrorObserverDoesNotReadRenderOrError() {
    LynxTemplateRender render = Mockito.mock(LynxTemplateRender.class);
    LynxError error = Mockito.mock(LynxError.class);
    LynxHostScriptViewHooks.onError(render, error);
    Mockito.verify(render, Mockito.never()).getLogContextSnapshot();
    Mockito.verify(error, Mockito.never()).getSummaryMessage();
    Mockito.verify(error, Mockito.never()).getLevel();
  }

  @Test
  public void enabledErrorObserverCopiesUnicodeWithoutChangingOriginal() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      setEnabled(true);
      LynxTemplateRender render = Mockito.mock(LynxTemplateRender.class);
      Mockito.when(render.getLogContextSnapshot()).thenReturn(new LynxLogContext(1, 2, 3));
      LynxError error = Mockito.mock(LynxError.class);
      Mockito.when(error.getSummaryMessage()).thenReturn("later error 🌍");
      Mockito.when(error.getErrorCode()).thenReturn(1001);
      Mockito.when(error.getLevel()).thenReturn(LynxError.LEVEL_WARN);
      LynxHostScriptViewHooks.onError(render, error);
      Mockito.verify(render).getLogContextSnapshot();
      Mockito.verify(error).getSummaryMessage();
      Mockito.verify(error).getErrorCode();
      Mockito.verify(error).getLevel();
      Mockito.verifyNoMoreInteractions(error);
      LynxHostScriptViewHooks.onError(null, null);
    });
  }

  @Test
  public void ssrAndHydrateSkipEnabledLoadInterceptor() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      setEnabled(true);
      LynxLoadMeta load = loadMeta();
      byte[] bytes = load.binaryData;
      for (LynxLoadMode mode :
          new LynxLoadMode[] {LynxLoadMode.RENDER_SSR, LynxLoadMode.HYDRATE_SSR}) {
        load.loadMode = mode;
        LynxHostScriptViewHooks.beforeLoadTemplate(null, load);
        assertSame(bytes, load.binaryData);
        assertEquals("assets://original.js", load.url);
      }
    });
  }

  @Test
  public void enabledInterceptorsWithoutHandlersKeepRequests() {
    TemplateBundle bundle = new TemplateBundle();
    assertFalse(bundle.isValid());
    try {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        setEnabled(true);
        LynxTemplateRender render = Mockito.mock(LynxTemplateRender.class);
        Mockito.when(render.getLogContextSnapshot()).thenReturn(new LynxLogContext(1, 2, 3));
        LynxLoadMeta load = loadMeta();
        load.initialData = TemplateData.fromString("{\"value\":1}");
        load.globalProps = TemplateData.fromString("{\"theme\":\"light\"}");
        load.byteBuffer = ByteBuffer.allocateDirect(8);
        load.bundle = bundle;
        TemplateData data = load.initialData;
        TemplateData props = load.globalProps;
        byte[] bytes = load.binaryData;
        ByteBuffer buffer = load.byteBuffer;
        // An invalid bundle must remain untouched when the dispatcher has no registered handler.
        LynxHostScriptViewHooks.beforeLoadTemplate(render, load);
        assertSame(bundle, load.bundle);
        load.bundle = null;
        LynxHostScriptViewHooks.beforeLoadTemplate(render, load);
        assertSame(buffer, load.byteBuffer);
        load.byteBuffer = null;
        LynxHostScriptViewHooks.beforeLoadTemplate(render, load);
        assertSame(bytes, load.binaryData);
        load.binaryData = null;
        load.url = null;
        LynxHostScriptViewHooks.beforeLoadTemplate(render, load);
        assertNull(load.url);
        assertSame(data, load.initialData);
        assertSame(props, load.globalProps);

        LynxUpdateMeta update =
            new LynxUpdateMeta.Builder().setUpdatedData(data).setUpdatedGlobalProps(props).build();
        assertSame(update, LynxHostScriptViewHooks.beforeUpdateMetaData(render, update));
        LynxUpdateMeta empty = new LynxUpdateMeta.Builder().build();
        assertSame(empty, LynxHostScriptViewHooks.beforeUpdateMetaData(render, empty));
      });
    } finally {
      bundle.release();
    }
  }

  @Test
  public void urlPatchClearsHigherPrioritySourcesAndKeepsOptions() throws Exception {
    LynxLoadMeta load = loadMeta();
    load.bundle = new TemplateBundle();
    load.byteBuffer = ByteBuffer.allocateDirect(8);
    TemplateData data = TemplateData.fromString("{\"value\":1}");
    load.initialData = data;
    load.loadOptions = LynxLoadOption.DUMP_ELEMENT.id();
    load.lynxViewConfig = Collections.singletonMap("enableLayoutOnly", "true");

    applyLoadPatch(load, "{\"url\":\"assets://replaced.js\",\"source\":{\"kind\":\"url\"}}", null);

    assertEquals("assets://replaced.js", load.url);
    assertNull(load.bundle);
    assertNull(load.byteBuffer);
    assertNull(load.binaryData);
    assertSame(data, load.initialData);
    assertEquals(LynxLoadMode.PRE_PAINTING, load.loadMode);
    assertTrue(load.enableDumpElementTree());
    assertEquals("true", load.lynxViewConfig.get("enableLayoutOnly"));
  }

  @Test
  public void bytePatchPreservesBinaryPayloadAndUnchangedUrl() throws Exception {
    LynxLoadMeta load = loadMeta();
    load.bundle = new TemplateBundle();
    load.byteBuffer = ByteBuffer.allocateDirect(8);
    byte[] replacement = new byte[] {0, (byte) 0xff, (byte) 0x80};

    applyLoadPatch(load, "{\"source\":{\"kind\":\"bytes\"}}", replacement);

    assertSame(replacement, load.binaryData);
    assertEquals("assets://original.js", load.url);
    assertNull(load.bundle);
    assertNull(load.byteBuffer);
  }

  @Test
  public void loadDataPatchPreservesProcessingStateAndTemplateSource() throws Exception {
    LynxLoadMeta load = loadMeta();
    load.initialData = TemplateData.fromString("{\"value\":\"original\"}");
    load.initialData.markState("processData");
    load.initialData.markReadOnly();
    load.globalProps = TemplateData.fromString("{\"theme\":\"light\"}");
    TemplateData original = load.initialData;
    TemplateData props = load.globalProps;
    byte[] bytes = load.binaryData;

    applyLoadPatch(load, "{\"initialData\":{\"value\":\"updated \\ud83d\\ude00\"}}", null);

    assertEquals("updated \uD83D\uDE00", load.initialData.toMap().get("value"));
    assertEquals("processData", load.initialData.processorName());
    assertTrue(load.initialData.isReadOnly());
    assertEquals("original", original.toMap().get("value"));
    assertSame(props, load.globalProps);
    assertSame(bytes, load.binaryData);
  }

  @Test
  public void loadPatchDistinguishesNullFromMissingFragments() throws Exception {
    LynxLoadMeta load = loadMeta();
    load.initialData = TemplateData.fromString("{\"value\":1}");
    load.globalProps = TemplateData.fromString("{\"theme\":\"light\"}");
    TemplateData data = load.initialData;
    applyLoadPatch(load, "{\"globalProps\":{\"theme\":\"dark\"}}", null);
    assertSame(data, load.initialData);
    assertEquals("dark", load.globalProps.toMap().get("theme"));
    applyLoadPatch(load, "{\"initialData\":null,\"globalProps\":null}", null);
    assertNull(load.initialData);
    assertNull(load.globalProps);
  }

  @Test
  public void malformedLoadPatchDoesNotPartiallyMutateRequest() throws Exception {
    LynxLoadMeta load = loadMeta();
    load.byteBuffer = ByteBuffer.allocateDirect(8);
    byte[] bytes = load.binaryData;
    ByteBuffer buffer = load.byteBuffer;
    try {
      applyLoadPatch(load, "{\"url\":\"assets://replaced.js\",\"source\":{}}", null);
      fail("Missing source kind must reject the whole patch");
    } catch (InvocationTargetException e) {
      assertTrue(e.getCause() instanceof JSONException);
    }
    assertEquals("assets://original.js", load.url);
    assertSame(bytes, load.binaryData);
    assertSame(buffer, load.byteBuffer);
  }

  @Test
  public void updatePatchReplacesDataWithoutMutatingOriginalFragments() throws Exception {
    TemplateData data = TemplateData.fromString("{\"value\":\"original\"}");
    data.markState("processData");
    data.markReadOnly();
    TemplateData props = TemplateData.fromString("{\"theme\":\"light\"}");
    LynxUpdateMeta original =
        new LynxUpdateMeta.Builder().setUpdatedData(data).setUpdatedGlobalProps(props).build();

    LynxUpdateMeta patched = applyUpdatePatch(original, "{\"updateData\":{\"value\":\"updated\"}}");

    assertEquals("updated", patched.getUpdatedData().toMap().get("value"));
    assertEquals("processData", patched.getUpdatedData().processorName());
    assertTrue(patched.getUpdatedData().isReadOnly());
    assertSame(props, patched.getUpdatedGlobalProps());
    assertSame(data, original.getUpdatedData());
    assertEquals("original", data.toMap().get("value"));
  }

  @Test
  public void updatePatchDistinguishesNullFromMissingFragments() throws Exception {
    TemplateData data = TemplateData.fromString("{\"value\":1}");
    TemplateData props = TemplateData.fromString("{\"theme\":\"light\"}");
    LynxUpdateMeta original =
        new LynxUpdateMeta.Builder().setUpdatedData(data).setUpdatedGlobalProps(props).build();
    LynxUpdateMeta unchanged = applyUpdatePatch(original, "{}");
    assertSame(data, unchanged.getUpdatedData());
    assertSame(props, unchanged.getUpdatedGlobalProps());
    LynxUpdateMeta patched = applyUpdatePatch(original, "{\"globalProps\":{\"theme\":\"dark\"}}");
    assertSame(data, patched.getUpdatedData());
    assertEquals("dark", patched.getUpdatedGlobalProps().toMap().get("theme"));
    LynxUpdateMeta cleared =
        applyUpdatePatch(original, "{\"updateData\":null,\"globalProps\":null}");
    assertNull(cleared.getUpdatedData());
    assertNull(cleared.getUpdatedGlobalProps());
  }

  @Test
  public void updatePatchCanAddPreviouslyAbsentFragments() throws Exception {
    LynxUpdateMeta original = new LynxUpdateMeta.Builder().build();
    LynxUpdateMeta patched = applyUpdatePatch(
        original, "{\"updateData\":{\"value\":1},\"globalProps\":{\"theme\":\"dark\"}}");
    assertEquals(1L, ((Number) patched.getUpdatedData().toMap().get("value")).longValue());
    assertFalse(patched.getUpdatedData().isReadOnly());
    assertNull(patched.getUpdatedData().processorName());
    assertEquals("dark", patched.getUpdatedGlobalProps().toMap().get("theme"));
    assertNull(original.getUpdatedData());
    assertNull(original.getUpdatedGlobalProps());
  }

  private static LynxLoadMeta loadMeta() {
    LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
    builder.setUrl("assets://original.js");
    builder.setBinaryData(new byte[] {1, 2, 3});
    builder.setLoadMode(LynxLoadMode.PRE_PAINTING);
    return builder.build();
  }

  private static void applyLoadPatch(LynxLoadMeta meta, String json, byte[] bytes)
      throws Exception {
    Method method = LynxHostScriptViewHooks.class.getDeclaredMethod(
        "applyLoadPatch", LynxLoadMeta.class, JSONObject.class, byte[].class);
    method.setAccessible(true);
    method.invoke(null, meta, new JSONObject(json), bytes);
  }

  private static LynxUpdateMeta applyUpdatePatch(LynxUpdateMeta meta, String json)
      throws Exception {
    Method method = LynxHostScriptViewHooks.class.getDeclaredMethod(
        "applyUpdatePatch", LynxUpdateMeta.class, JSONObject.class);
    method.setAccessible(true);
    return (LynxUpdateMeta) method.invoke(null, meta, new JSONObject(json));
  }

  private void queueCreation(boolean enabled, Runnable beforeDelivery) {
    AtomicReference<Throwable> failure = new AtomicReference<>();
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      setEnabled(enabled);
      // Hold the UI queue until the worker has posted and the state has changed.
      Thread worker = new Thread(() -> {
        try {
          LynxHostScriptViewHooks.postCreationNotification(mNotifications::incrementAndGet);
        } catch (Throwable throwable) {
          failure.set(throwable);
        }
      });
      worker.start();
      try {
        worker.join(3000);
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
        throw new AssertionError(e);
      }
      assertFalse(worker.isAlive());
      beforeDelivery.run();
    });
    InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    assertNull(failure.get());
  }

  private void setEnabled(boolean enabled) {
    try {
      mSetEnabled.invoke(null, enabled);
    } catch (ReflectiveOperationException e) {
      throw new AssertionError(e);
    }
  }
}
