// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.app.Application;
import android.content.Context;
import android.view.View.MeasureSpec;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.After;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;

@RunWith(AndroidJUnit4.class)
public class LynxHostScriptViewHooksTest {
  private Method mSetEnabled;
  private boolean mWasEnabled;
  private boolean mHostScriptInstalled;

  @Before
  public void setUp() throws Exception {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
    mSetEnabled = LynxHostScriptViewHooks.class.getDeclaredMethod("setEnabled", boolean.class);
    mSetEnabled.setAccessible(true);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      mHostScriptInstalled = isHostScriptAvailable();
      mWasEnabled = LynxHostScriptViewHooks.isEnabled();
      setEnabled(false);
    });
  }

  @After
  public void tearDown() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> { setEnabled(mWasEnabled); });
  }

  @Test
  public void engineRebuildKeepsViewRegisteredUntilViewIsDestroyed() throws Exception {
    Assume.assumeTrue(mHostScriptInstalled);
    Field registryAvailable = LynxHostScriptViewHooks.class.getDeclaredField("sRegistryAvailable");
    registryAvailable.setAccessible(true);
    Assume.assumeTrue(registryAvailable.getBoolean(null));
    Method reload = LynxTemplateRender.class.getDeclaredMethod("reloadAndInit");
    reload.setAccessible(true);
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      LynxView view = new LynxViewBuilder().build(context);
      LynxTemplateRender render = view.mLynxTemplateRender;
      long viewId = render.getLogContextSnapshot().viewId;
      AtomicInteger engineDestructions = new AtomicInteger();
      view.addLynxViewClient(new LynxViewClient() {
        @Override
        public void onDestroy() {
          engineDestructions.incrementAndGet();
        }
      });
      try {
        assertTrue(nativeIsViewRegistered(viewId));
        // The first load marks reload; the next load replaces the engine.
        reload.invoke(render);
        reload.invoke(render);
        assertEquals(1, engineDestructions.get());
        assertEquals(viewId, render.getLogContextSnapshot().viewId);
        assertTrue(nativeIsViewRegistered(viewId));
      } catch (ReflectiveOperationException error) {
        throw new AssertionError(error);
      } finally {
        view.destroy();
      }
      assertEquals(2, engineDestructions.get());
      assertFalse(nativeIsViewRegistered(viewId));
      view.destroy();
      assertFalse(nativeIsViewRegistered(viewId));
    });
  }

  private static native boolean nativeIsViewRegistered(long viewId);

  @Test
  public void scriptOriginMarksOnlyTheFirstOperationOfItsView() {
    List<Boolean> origins = new ArrayList<>();
    LynxHostScriptViewHooks.runScriptCommand(7, true, () -> {
      origins.add(LynxHostScriptViewHooks.consumeScriptOrigin(8));
      origins.add(LynxHostScriptViewHooks.consumeScriptOrigin(7));
      // A nested host operation on the same View keeps origin "host".
      origins.add(LynxHostScriptViewHooks.consumeScriptOrigin(7));
    });
    origins.add(LynxHostScriptViewHooks.consumeScriptOrigin(7));
    LynxHostScriptViewHooks.runScriptCommand(
        7, false, () -> origins.add(LynxHostScriptViewHooks.consumeScriptOrigin(7)));
    assertEquals(Arrays.asList(false, true, false, false, false), origins);
  }

  @Test
  public void nestedScriptCommandsRestoreTheOuterOrigin() {
    List<Boolean> origins = new ArrayList<>();
    LynxHostScriptViewHooks.runScriptCommand(7, true, () -> {
      LynxHostScriptViewHooks.runScriptCommand(
          8, true, () -> origins.add(LynxHostScriptViewHooks.consumeScriptOrigin(8)));
      origins.add(LynxHostScriptViewHooks.consumeScriptOrigin(7));
    });
    assertEquals(Arrays.asList(true, true), origins);
  }

  @Test
  public void disabledInterceptorsKeepRequestsWithoutAccessingRender() {
    LynxLoadMeta load = loadMeta();
    TemplateData data = TemplateData.fromString("{\"value\":1}");
    TemplateData props = TemplateData.fromString("{\"theme\":\"light\"}");
    load.initialData = data;
    LynxUpdateMeta update = new LynxUpdateMeta.Builder().setUpdatedData(data).build();
    LynxViewBuilder builder = new LynxViewBuilder();
    int width = MeasureSpec.makeMeasureSpec(720, MeasureSpec.EXACTLY);
    int height = MeasureSpec.makeMeasureSpec(1280, MeasureSpec.AT_MOST);

    LynxHostScriptViewHooks.beforeLoadTemplate(null, load);
    assertSame(data, load.initialData);
    assertSame(update, LynxHostScriptViewHooks.beforeUpdateMetaData(null, update));
    assertSame(props, LynxHostScriptViewHooks.beforeUpdateGlobalProps(null, props));
    assertSame(builder, LynxHostScriptViewHooks.beforeCreate(null, builder));
    long packed = LynxHostScriptViewHooks.beforeUpdateViewport(null, width, height);
    assertEquals(width, (int) (packed >>> 32));
    assertEquals(height, (int) packed);
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
    Assume.assumeTrue(mHostScriptInstalled);
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
        assertSame(props, LynxHostScriptViewHooks.beforeUpdateGlobalProps(render, props));
        assertNull(LynxHostScriptViewHooks.beforeUpdateGlobalProps(render, null));

        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        LynxViewBuilder builder = new LynxViewBuilder();
        assertNull(LynxHostScriptViewHooks.beforeCreate(context, null));
        assertSame(builder, LynxHostScriptViewHooks.beforeCreate(context, builder));

        int width = MeasureSpec.makeMeasureSpec(720, MeasureSpec.EXACTLY);
        int height = MeasureSpec.makeMeasureSpec(1280, MeasureSpec.AT_MOST);
        long packed = LynxHostScriptViewHooks.beforeUpdateViewport(render, width, height);
        assertEquals(width, (int) (packed >>> 32));
        assertEquals(height, (int) packed);
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

  @Test
  public void createRequestAndPatchHelpersPreserveAndIsolateBuilderOptions() throws Exception {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    LynxViewBuilder defaults = new LynxViewBuilder();
    defaults.lynxViewConfig = null;
    defaults.setScreenSize(720, 0);
    JSONObject minimal = createRequest(context, defaults);
    assertEquals(ThreadStrategyForRendering.ALL_ON_UI.id(), minimal.getInt("threadStrategy"));
    assertTrue(minimal.getBoolean("enableJSRuntime"));
    assertFalse(minimal.getBoolean("enableAirStrictMode"));
    assertFalse(minimal.getBoolean("enableMTSModule"));
    assertFalse(minimal.has("presetMeasuredSpec"));
    assertFalse(minimal.has("screenSize"));
    assertFalse(minimal.has("density"));
    assertFalse(minimal.has("lynxViewConfig"));
    assertEquals(1.0, minimal.getDouble("fontScale"), 0.001);
    assertFalse(minimal.getBoolean("forceDarkAllowed"));
    assertTrue(minimal.getJSONObject("deviceScreenSize").getInt("width") > 0);
    assertTrue(minimal.getJSONObject("deviceScreenSize").getInt("height") > 0);

    LynxViewBuilder emptyConfigCopy = copyBuilder(defaults);
    assertNotSame(defaults, emptyConfigCopy);
    assertNotNull(emptyConfigCopy.getLynxViewConfig());
    assertTrue(emptyConfigCopy.getLynxViewConfig().isEmpty());

    int widthSpec = MeasureSpec.makeMeasureSpec(720, MeasureSpec.EXACTLY);
    int heightSpec = MeasureSpec.makeMeasureSpec(1280, MeasureSpec.AT_MOST);
    LynxViewBuilder configured =
        new LynxViewBuilder()
            .setThreadStrategyForRendering(ThreadStrategyForRendering.MOST_ON_TASM)
            .setEnableJSRuntime(false)
            .setEnableAirStrictMode(true)
            .setEnableMTSModule(true)
            .setPresetMeasuredSpec(widthSpec, heightSpec)
            .setScreenSize(1080, 1920)
            .setDensity(3.0f)
            .setFontScale(1.25f)
            .setForceDarkAllowed(true)
            .setLynxViewConfig(Collections.singletonMap("host", "yes"));
    JSONObject full = createRequest(context, configured);
    assertEquals(ThreadStrategyForRendering.MOST_ON_TASM.id(), full.getInt("threadStrategy"));
    assertFalse(full.getBoolean("enableJSRuntime"));
    assertTrue(full.getBoolean("enableAirStrictMode"));
    assertTrue(full.getBoolean("enableMTSModule"));
    assertEquals(widthSpec, full.getJSONObject("presetMeasuredSpec").getInt("width"));
    assertEquals(heightSpec, full.getJSONObject("presetMeasuredSpec").getInt("height"));
    assertEquals(1080, full.getJSONObject("screenSize").getInt("width"));
    assertEquals(1920, full.getJSONObject("screenSize").getInt("height"));
    assertEquals(3.0, full.getDouble("density"), 0.001);
    assertEquals(1.25, full.getDouble("fontScale"), 0.001);
    assertTrue(full.getBoolean("forceDarkAllowed"));
    assertEquals("yes", full.getJSONObject("lynxViewConfig").getString("host"));

    LynxViewBuilder copy = copyBuilder(configured);
    assertNotSame(configured, copy);
    assertNotSame(configured.getLynxRuntimeOptions(), copy.getLynxRuntimeOptions());
    assertNotSame(configured.getLynxViewConfig(), copy.getLynxViewConfig());

    applyCreatePatch(copy, "{}");
    assertEquals(ThreadStrategyForRendering.MOST_ON_TASM, copy.getThreadStrategy());
    assertFalse(copy.isEnableJSRuntime());
    assertEquals("yes", copy.getLynxViewConfig().get("host"));

    int patchedWidth = MeasureSpec.makeMeasureSpec(600, MeasureSpec.EXACTLY);
    int patchedHeight = MeasureSpec.makeMeasureSpec(900, MeasureSpec.EXACTLY);
    applyCreatePatch(copy,
        "{\"threadStrategy\":" + ThreadStrategyForRendering.PART_ON_LAYOUT.id()
            + ",\"enableJSRuntime\":true,\"enableAirStrictMode\":false,\"enableMTSModule\":false,"
            + "\"forceDarkAllowed\":false,\"presetMeasuredSpec\":{\"width\":" + patchedWidth
            + ",\"height\":" + patchedHeight + "},\"screenSize\":{\"width\":600,\"height\":900},"
            + "\"density\":2.5,\"fontScale\":1.5,\"lynxViewConfig\":{\"patch\":\"applied\"}}");
    assertEquals(ThreadStrategyForRendering.PART_ON_LAYOUT, copy.getThreadStrategy());
    assertTrue(copy.isEnableJSRuntime());
    assertFalse(copy.isEnableAirStrictMode());
    assertFalse(copy.isEnableMTSModule());
    assertFalse(copy.getForceDarkAllowed());
    assertEquals(patchedWidth, copy.getPresetWidthMeasureSpec());
    assertEquals(patchedHeight, copy.getPresetHeightMeasureSpec());
    assertEquals(600, copy.getScreenWidth());
    assertEquals(900, copy.getScreenHeight());
    assertEquals(2.5f, copy.getDensity(), 0.001f);
    assertEquals(1.5f, copy.getFontScale(), 0.001f);
    assertEquals("yes", copy.getLynxViewConfig().get("host"));
    assertEquals("applied", copy.getLynxViewConfig().get("patch"));
    assertEquals(1, configured.getLynxViewConfig().size());
    assertFalse(configured.isEnableJSRuntime());
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

  private static LynxViewBuilder copyBuilder(LynxViewBuilder source) throws Exception {
    Method method =
        LynxHostScriptViewHooks.class.getDeclaredMethod("copyBuilder", LynxViewBuilder.class);
    method.setAccessible(true);
    return (LynxViewBuilder) method.invoke(null, source);
  }

  private static JSONObject createRequest(Context context, LynxViewBuilder builder)
      throws Exception {
    Method method = LynxHostScriptViewHooks.class.getDeclaredMethod(
        "createRequest", Context.class, LynxViewBuilder.class);
    method.setAccessible(true);
    return (JSONObject) method.invoke(null, context, builder);
  }

  private static void applyCreatePatch(LynxViewBuilder builder, String json) throws Exception {
    Method method = LynxHostScriptViewHooks.class.getDeclaredMethod(
        "applyCreatePatch", LynxViewBuilder.class, JSONObject.class);
    method.setAccessible(true);
    method.invoke(null, builder, new JSONObject(json));
  }

  private void setEnabled(boolean enabled) {
    try {
      mSetEnabled.invoke(null, enabled);
    } catch (ReflectiveOperationException e) {
      throw new AssertionError(e);
    }
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
}
