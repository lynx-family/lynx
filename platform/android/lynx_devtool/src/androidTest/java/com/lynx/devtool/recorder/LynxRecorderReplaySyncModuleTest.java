// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import android.content.Context;
import android.util.DisplayMetrics;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.testing.base.TestingLynxContext;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxRecorderReplaySyncModuleTest {
  private static final String APPLET_BRIDGE_EVENT = "__APPLET_BRIDGE__";

  private Context mBaseContext;
  private DisplayMetrics mDisplayMetrics;

  @Before
  public void setUp() {
    mBaseContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    mDisplayMetrics = new DisplayMetrics();
    mDisplayMetrics.widthPixels = 1080;
    mDisplayMetrics.heightPixels = 1920;
    mDisplayMetrics.density = 3.0f;
  }

  @Test
  public void emitReplayGlobalEvent_remapsCallbackIdBeforeDispatch() throws JSONException {
    MockLynxContext context = new MockLynxContext(mBaseContext, mDisplayMetrics);
    LynxRecorderReplaySyncModule module =
        new LynxRecorderReplaySyncModule(context, new FakeReplayDataProvider(buildActionList(25)));

    module.emitReplayGlobalEvent(buildParams("25_emitReplayGlobalEvent", 21));

    assertEquals(APPLET_BRIDGE_EVENT, context.lastEventName);
    assertNotNull(context.lastParams);
    JavaOnlyMap callbackPayload = context.lastParams.getMap(0);
    assertEquals(21L, callbackPayload.getLong("callbackId"));
    assertEquals("callback", callbackPayload.getString("type"));
    assertEquals(
        "{\"status\":0}", callbackPayload.getMap("params").getMap("data").getString("result"));
  }

  @Test
  public void emitReplayGlobalEvent_normalizesCallbackIdAcrossLabelRuntimeAndPayload()
      throws JSONException {
    MockLynxContext context = new MockLynxContext(mBaseContext, mDisplayMetrics);
    LynxRecorderReplaySyncModule module = new LynxRecorderReplaySyncModule(
        context, new FakeReplayDataProvider(buildActionList(25.0d)));

    module.emitReplayGlobalEvent(buildParams("25.0_emitReplayGlobalEvent", "21.0"));

    assertEquals(APPLET_BRIDGE_EVENT, context.lastEventName);
    assertNotNull(context.lastParams);
    JavaOnlyMap callbackPayload = context.lastParams.getMap(0);
    assertEquals(21.0d, ((Number) callbackPayload.get("callbackId")).doubleValue(), 0.0d);
  }

  @Test
  public void emitReplayGlobalEvent_skipsDispatchWhenRecordedCallbackIdIsMissing()
      throws JSONException {
    MockLynxContext context = new MockLynxContext(mBaseContext, mDisplayMetrics);
    LynxRecorderReplaySyncModule module =
        new LynxRecorderReplaySyncModule(context, new FakeReplayDataProvider(buildActionList(25)));

    module.emitReplayGlobalEvent(buildParams("default", 21));

    assertNull(context.lastEventName);
    assertNull(context.lastParams);
  }

  private static JavaOnlyMap buildParams(String label, Object runtimeCallbackId) {
    JavaOnlyMap params = new JavaOnlyMap();
    params.put("label", label);

    JavaOnlyMap runtimeArg = new JavaOnlyMap();
    runtimeArg.put("callbackId", runtimeCallbackId);

    JavaOnlyArray args = new JavaOnlyArray();
    args.pushMap(runtimeArg);
    params.putArray("args", args);
    return params;
  }

  private static JSONArray buildActionList(Object callbackId) throws JSONException {
    JSONObject payload = new JSONObject();
    payload.put("callbackId", callbackId);
    payload.put("type", "callback");

    JSONObject payloadData = new JSONObject();
    payloadData.put("result", "{\"status\":0}");

    JSONObject payloadParams = new JSONObject();
    payloadParams.put("data", payloadData);
    payload.put("params", payloadParams);

    JSONArray payloads = new JSONArray();
    payloads.put(payload);

    JSONArray arguments = new JSONArray();
    arguments.put(APPLET_BRIDGE_EVENT);
    arguments.put(payloads);

    JSONObject actionParams = new JSONObject();
    actionParams.put("module_id", "GlobalEventEmitter");
    actionParams.put("method_id", "emit");
    actionParams.put("arguments", arguments);

    JSONObject action = new JSONObject();
    action.put("Function Name", "sendGlobalEvent");
    action.put("Params", actionParams);

    return new JSONArray().put(action);
  }

  private static class FakeReplayDataProvider implements LynxRecorderReplayDataProvider {
    private final JSONArray mActionList;

    FakeReplayDataProvider(JSONArray actionList) {
      mActionList = actionList;
    }

    @Override
    public JSONArray getFunctionCall() {
      return null;
    }

    @Override
    public JSONObject getCallbackData() {
      return null;
    }

    @Override
    public JSONArray getActionList() {
      return mActionList;
    }

    @Override
    public JSONArray getJsbIgnoredInfo() {
      return null;
    }

    @Override
    public JSONObject getJsbSettings() {
      return null;
    }

    @Override
    public JSONObject getSharedData() {
      return null;
    }
  }

  private static class MockLynxContext extends TestingLynxContext {
    private String lastEventName;
    private JavaOnlyArray lastParams;

    MockLynxContext(Context base, DisplayMetrics screenMetrics) {
      super(base, screenMetrics);
    }

    @Override
    public void sendGlobalEvent(String name, JavaOnlyArray params) {
      lastEventName = name;
      lastParams = params;
    }
  }
}
