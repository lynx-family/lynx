// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import com.lynx.jsbridge.LynxContextModule;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import java.math.BigDecimal;
import java.util.Iterator;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class LynxRecorderReplaySyncModule extends LynxContextModule {
  private static final String TAG = "LynxRecorderReplaySyncModule";
  private static final String GLOBAL_EVENT_EMITTER = "GlobalEventEmitter";
  private static final String EMIT_METHOD = "emit";
  private static final String APPLET_BRIDGE_EVENT = "__APPLET_BRIDGE__";
  private static final String CALLBACK_TYPE = "callback";

  private final JSONArray mActionList;

  public LynxRecorderReplaySyncModule(LynxContext context) {
    super(context);
    mActionList = null;
  }

  public LynxRecorderReplaySyncModule(LynxContext context, Object param) {
    super(context, param);
    LynxRecorderReplayDataProvider provider = (LynxRecorderReplayDataProvider) param;
    mActionList = provider.getActionList();
  }

  @LynxMethod
  public void emitReplayGlobalEvent(ReadableMap params) {
    if (mLynxContext == null || params == null) {
      return;
    }
    String recordedCallbackId = extractCallbackIdFromLabel(params);
    if (recordedCallbackId == null) {
      return;
    }
    String runtimeCallbackId = extractCallbackIdFromRuntimeArgs(params.getArray("args", null));
    try {
      JSONArray payload = findCallbackPayload(recordedCallbackId);
      if (payload == null) {
        return;
      }
      mLynxContext.sendGlobalEvent(APPLET_BRIDGE_EVENT,
          jsonArrayToJavaOnlyArray(remapCallbackId(payload, runtimeCallbackId)));
    } catch (JSONException e) {
      LLog.e(TAG, "emitReplayGlobalEvent failed");
    }
  }

  private String extractCallbackIdFromLabel(ReadableMap params) {
    Object label = params.asHashMap().get("label");
    if (label == null) {
      return null;
    }
    String labelText = String.valueOf(label);
    if (labelText.isEmpty() || "default".equals(labelText)) {
      return null;
    }
    int separatorIndex = labelText.indexOf('_');
    return normalizeCallbackId(
        separatorIndex >= 0 ? labelText.substring(0, separatorIndex) : labelText);
  }

  private String extractCallbackIdFromRuntimeArgs(ReadableArray args) {
    if (args == null || args.size() == 0) {
      return null;
    }
    ReadableMap firstArg = args.getMap(0);
    if (firstArg == null || !firstArg.hasKey("callbackId")) {
      return null;
    }
    return normalizeCallbackId(firstArg.asHashMap().get("callbackId"));
  }

  private String normalizeCallbackId(Object callbackId) {
    if (callbackId == null || callbackId == JSONObject.NULL) {
      return null;
    }
    String value = String.valueOf(callbackId);
    if (value.isEmpty()) {
      return null;
    }
    try {
      return new BigDecimal(value).stripTrailingZeros().toPlainString();
    } catch (NumberFormatException ignore) {
      return value;
    }
  }

  private boolean isSameCallbackId(String expectedCallbackId, Object actualCallbackId) {
    if (expectedCallbackId == null) {
      return false;
    }
    return expectedCallbackId.equals(normalizeCallbackId(actualCallbackId));
  }

  private JSONArray findCallbackPayload(String callbackId) throws JSONException {
    if (mActionList == null) {
      return null;
    }
    for (int actionIndex = 0; actionIndex < mActionList.length(); actionIndex++) {
      JSONObject action = mActionList.optJSONObject(actionIndex);
      if (!isTargetCallbackAction(action)) {
        continue;
      }
      JSONArray payloads = getCallbackPayloads(action);
      if (payloads == null) {
        continue;
      }
      for (int payloadIndex = 0; payloadIndex < payloads.length(); payloadIndex++) {
        JSONObject payload = payloads.optJSONObject(payloadIndex);
        if (payload == null || !CALLBACK_TYPE.equals(payload.optString("type"))) {
          continue;
        }
        if (isSameCallbackId(callbackId, payload.opt("callbackId"))) {
          return new JSONArray().put(new JSONObject(payload.toString()));
        }
      }
    }
    return null;
  }

  private JSONArray remapCallbackId(JSONArray payloads, String runtimeCallbackId)
      throws JSONException {
    JSONArray remapped = new JSONArray(payloads.toString());
    if (runtimeCallbackId == null) {
      return remapped;
    }
    for (int index = 0; index < remapped.length(); index++) {
      JSONObject payload = remapped.optJSONObject(index);
      if (payload == null || !payload.has("callbackId")) {
        continue;
      }
      if (isSameCallbackId(runtimeCallbackId, payload.opt("callbackId"))) {
        continue;
      }
      try {
        payload.put("callbackId", Long.parseLong(runtimeCallbackId));
      } catch (NumberFormatException longError) {
        try {
          payload.put("callbackId", Double.parseDouble(runtimeCallbackId));
        } catch (NumberFormatException doubleError) {
          LLog.e(TAG,
              "remapCallbackId failed, runtimeCallbackId is not numeric: " + runtimeCallbackId);
        }
      }
    }
    return remapped;
  }

  private boolean isTargetCallbackAction(JSONObject action) {
    if (action == null || !"sendGlobalEvent".equals(action.optString("Function Name"))) {
      return false;
    }
    JSONObject params = action.optJSONObject("Params");
    if (params == null) {
      return false;
    }
    if (!GLOBAL_EVENT_EMITTER.equals(params.optString("module_id"))
        || !EMIT_METHOD.equals(params.optString("method_id"))) {
      return false;
    }
    JSONArray arguments = params.optJSONArray("arguments");
    return arguments != null && arguments.length() >= 2
        && APPLET_BRIDGE_EVENT.equals(arguments.optString(0));
  }

  private JSONArray getCallbackPayloads(JSONObject action) {
    JSONObject params = action.optJSONObject("Params");
    if (params == null) {
      return null;
    }
    JSONArray arguments = params.optJSONArray("arguments");
    if (arguments == null || arguments.length() < 2) {
      return null;
    }
    return arguments.optJSONArray(1);
  }

  private static JavaOnlyArray jsonArrayToJavaOnlyArray(JSONArray array) throws JSONException {
    JavaOnlyArray result = new JavaOnlyArray();
    for (int index = 0; index < array.length(); index++) {
      Object value = array.opt(index);
      if (value instanceof JSONObject) {
        result.pushMap(jsonObjectToJavaOnlyMap((JSONObject) value));
      } else if (value instanceof JSONArray) {
        result.pushArray(jsonArrayToJavaOnlyArray((JSONArray) value));
      } else if (value == JSONObject.NULL) {
        result.pushNull();
      } else {
        result.add(value);
      }
    }
    return result;
  }

  private static JavaOnlyMap jsonObjectToJavaOnlyMap(JSONObject object) throws JSONException {
    JavaOnlyMap result = new JavaOnlyMap();
    Iterator<String> iterator = object.keys();
    while (iterator.hasNext()) {
      String key = iterator.next();
      Object value = object.opt(key);
      if (value instanceof JSONObject) {
        result.put(key, jsonObjectToJavaOnlyMap((JSONObject) value));
      } else if (value instanceof JSONArray) {
        result.put(key, jsonArrayToJavaOnlyArray((JSONArray) value));
      } else {
        result.put(key, value);
      }
    }
    return result;
  }
}
