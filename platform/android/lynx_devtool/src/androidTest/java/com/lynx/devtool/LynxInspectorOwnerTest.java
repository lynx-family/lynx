// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.devtool.helper.HostScreencast;
import com.lynx.devtoolwrapper.DevToolSettings;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxView;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import java.util.Map;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxInspectorOwnerTest {
  private LynxInspectorOwner mInspectorOwner;
  private Context mContext;

  @Before
  public void setUp() {
    mContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    // Initialize LynxEnv and DevToolSettings (required for get/setGlobalSwitch)
    LynxEnv.inst().init((android.app.Application) mContext, System::loadLibrary, null, null);
    DevToolSettings.inst().init(mContext);

    // Initialize inspector owner (debuggable=true)
    mInspectorOwner = new LynxInspectorOwner(true);
  }

  @Test
  public void testHostPreviewTracksLateBoundAndReboundView() throws Exception {
    LynxView[] views = new LynxView[2];
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      // Establish the session before a view exists, as with preloaded templates.
      mInspectorOwner.attachToDebugBridge("assets://preview-test.js");
      views[0] = new LynxView(mContext);
      views[1] = new LynxView(mContext);
      mInspectorOwner.attach(views[0]);
    });
    Field delegateField = LynxInspectorOwner.class.getDeclaredField("mLynxDevToolNG");
    delegateField.setAccessible(true);
    int id = ((LynxDevToolNGDelegate) delegateField.get(mInspectorOwner)).getSessionId();
    assertTrue(id > 0);
    Field instanceField = HostScreencast.class.getDeclaredField("INSTANCE");
    instanceField.setAccessible(true);
    Field viewsField = HostScreencast.class.getDeclaredField("mViews");
    viewsField.setAccessible(true);
    @SuppressWarnings("unchecked")
    Map<Integer, WeakReference<LynxView>> registered =
        (Map<Integer, WeakReference<LynxView>>) viewsField.get(instanceField.get(null));
    try {
      InstrumentationRegistry.getInstrumentation().waitForIdleSync();
      InstrumentationRegistry.getInstrumentation().runOnMainSync(
          () -> assertSame(views[0], registered.get(id).get()));
      InstrumentationRegistry.getInstrumentation().runOnMainSync(
          () -> mInspectorOwner.attach(views[1]));
      InstrumentationRegistry.getInstrumentation().waitForIdleSync();
      InstrumentationRegistry.getInstrumentation().runOnMainSync(
          () -> assertSame(views[1], registered.get(id).get()));
    } finally {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        mInspectorOwner.destroy();
        views[0].destroy();
        views[1].destroy();
      });
      InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    }
    InstrumentationRegistry.getInstrumentation().runOnMainSync(
        () -> assertFalse(registered.containsKey(id)));
  }

  @Test
  public void testSetGlobalSwitch_EnableDevTool() throws JSONException {
    // Construct message for enabling devtool
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_DEVTOOL);
    message.put("global_value", true);

    // Call setGlobalSwitch
    Object result = mInspectorOwner.setGlobalSwitch(message.toString());

    // Verify result and side effect
    assertEquals(Boolean.TRUE, result);
    assertTrue(DevToolSettings.inst().isDevToolEnabled());
  }

  @Test
  public void testSetGlobalSwitch_DisableDevTool() throws JSONException {
    // Construct message for disabling devtool
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_DEVTOOL);
    message.put("global_value", false);

    // Call setGlobalSwitch
    Object result = mInspectorOwner.setGlobalSwitch(message.toString());

    // Verify result and side effect
    assertEquals(Boolean.FALSE, result);
    assertFalse(DevToolSettings.inst().isDevToolEnabled());
  }

  @Test
  public void testGetGlobalSwitch_EnableDevTool() throws JSONException {
    // Pre-set value
    DevToolSettings.inst().setDevToolEnabled(true);

    // Construct message for getting devtool status
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_DEVTOOL);

    // Call getGlobalSwitch
    Object result = mInspectorOwner.getGlobalSwitch(message.toString());

    // Verify result
    assertTrue((Boolean) result);
  }

  @Test
  public void testSetGlobalSwitch_UnsupportedKey() throws JSONException {
    // Construct message with unsupported key
    JSONObject message = new JSONObject();
    message.put("global_key", "unsupported_key");
    message.put("global_value", true);

    // Call setGlobalSwitch
    Object result = mInspectorOwner.setGlobalSwitch(message.toString());

    // Verify result is null (or handled gracefully)
    assertNull(result);
  }

  @Test
  public void testSetGlobalSwitch_EnableLogBox() throws JSONException {
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_LOGBOX);
    message.put("global_value", true);

    Object result = mInspectorOwner.setGlobalSwitch(message.toString());

    assertEquals(Boolean.TRUE, result);
    assertTrue(DevToolSettings.inst().isLogBoxEnabled());
  }

  @Test
  public void testGetGlobalSwitch_EnableLogBox() throws JSONException {
    DevToolSettings.inst().setLogBoxEnabled(true);
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_LOGBOX);

    Object result = mInspectorOwner.getGlobalSwitch(message.toString());
    assertTrue((Boolean) result);
  }

  @Test
  public void testSetGlobalSwitch_EnableDebugMode() throws JSONException {
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_DEBUG_MODE);
    message.put("global_value", true);

    Object result = mInspectorOwner.setGlobalSwitch(message.toString());

    assertEquals(Boolean.TRUE, result);
    assertTrue(DevToolSettings.inst().isDebugModeEnabled());
  }

  @Test
  public void testGetGlobalSwitch_EnableDebugMode() throws JSONException {
    DevToolSettings.inst().setDebugModeEnabled(true);
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_DEBUG_MODE);

    Object result = mInspectorOwner.getGlobalSwitch(message.toString());
    assertTrue((Boolean) result);
  }

  @Test
  public void testSetGlobalSwitch_EnableQuickJSDebug() throws JSONException {
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_QUICKJS_DEBUG);
    message.put("global_value", true);

    Object result = mInspectorOwner.setGlobalSwitch(message.toString());

    assertEquals(Boolean.TRUE, result);
    assertTrue(DevToolSettings.inst().isQuickJSDebugEnabled());
  }

  @Test
  public void testGetGlobalSwitch_EnableQuickJSDebug() throws JSONException {
    DevToolSettings.inst().setQuickJSDebugEnabled(true);
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_QUICKJS_DEBUG);

    Object result = mInspectorOwner.getGlobalSwitch(message.toString());
    assertTrue((Boolean) result);
  }

  @Test
  public void testSetGlobalSwitch_EnableV8() throws JSONException {
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_V8);
    // V8 uses integer value
    message.put("global_value", 1);

    Object result = mInspectorOwner.setGlobalSwitch(message.toString());

    assertEquals(1, result);
    assertEquals(1, DevToolSettings.inst().getV8Enabled());
  }

  @Test
  public void testGetGlobalSwitch_EnableV8() throws JSONException {
    DevToolSettings.inst().setV8Enabled(1);
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_V8);

    Object result = mInspectorOwner.getGlobalSwitch(message.toString());
    assertEquals(1, result);
  }

  @Test
  public void testSetGlobalSwitch_EnableDomTree() throws JSONException {
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_DOM_TREE);
    message.put("global_value", true);

    Object result = mInspectorOwner.setGlobalSwitch(message.toString());

    assertEquals(Boolean.TRUE, result);
    assertTrue(DevToolSettings.inst().isDOMTreeEnabled());
  }

  @Test
  public void testGetGlobalSwitch_EnableDomTree() throws JSONException {
    DevToolSettings.inst().setDOMTreeEnabled(true);
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_DOM_TREE);

    Object result = mInspectorOwner.getGlobalSwitch(message.toString());
    assertTrue((Boolean) result);
  }

  @Test
  public void testSetGlobalSwitch_EnableLongPressMenu() throws JSONException {
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_LONG_PRESS_MENU);
    message.put("global_value", true);

    Object result = mInspectorOwner.setGlobalSwitch(message.toString());

    assertEquals(Boolean.TRUE, result);
    assertTrue(DevToolSettings.inst().isLongPressMenuEnabled());
  }

  @Test
  public void testGetGlobalSwitch_EnableLongPressMenu() throws JSONException {
    DevToolSettings.inst().setLongPressMenuEnabled(true);
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_LONG_PRESS_MENU);

    Object result = mInspectorOwner.getGlobalSwitch(message.toString());
    assertTrue((Boolean) result);
  }

  @Test
  public void testSetGlobalSwitch_EnablePixelCopy() throws JSONException {
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_PIXEL_COPY);
    message.put("global_value", true);

    Object result = mInspectorOwner.setGlobalSwitch(message.toString());

    assertEquals(Boolean.TRUE, result);
    assertTrue(DevToolSettings.inst().isPixelCopyEnabled());
  }

  @Test
  public void testGetGlobalSwitch_EnablePixelCopy() throws JSONException {
    DevToolSettings.inst().setPixelCopyEnabled(true);
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_PIXEL_COPY);

    Object result = mInspectorOwner.getGlobalSwitch(message.toString());
    assertTrue((Boolean) result);
  }

  @Test
  public void testSetGlobalSwitch_EnablePerfMetrics() throws JSONException {
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_PERF_METRICS);
    message.put("global_value", true);

    Object result = mInspectorOwner.setGlobalSwitch(message.toString());

    assertEquals(Boolean.TRUE, result);
    assertTrue(DevToolSettings.inst().isPerfMetricsEnabled());
  }

  @Test
  public void testGetGlobalSwitch_EnablePerfMetrics() throws JSONException {
    DevToolSettings.inst().setPerfMetricsEnabled(true);
    JSONObject message = new JSONObject();
    message.put("global_key", DevToolSettings.SP_KEY_ENABLE_PERF_METRICS);

    Object result = mInspectorOwner.getGlobalSwitch(message.toString());
    assertTrue((Boolean) result);
  }

  @Test
  public void testCDPMethodsAfterDestroyAreNoOp() {
    mInspectorOwner.destroy();

    mInspectorOwner.invokeCDPFromSDK("", result -> {});
    mInspectorOwner.addCDPEventListener("test", event -> {});
    mInspectorOwner.removeCDPEventListener("test");
  }
}
