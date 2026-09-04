// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import com.lynx.jsbridge.LynxContextModule;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.PiperData;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class LynxRecorderReplayDataModule extends LynxContextModule {
  private static final String TAG = "LynxRecorderReplayDataModule";
  private static final String APPLET_BRIDGE_MODULE = "AppletBridgeModule";
  private static final String POST_MESSAGE_METHOD = "postMessage";
  private static final String GLOBAL_EVENT_EMITTER = "GlobalEventEmitter";
  private static final String EMIT_METHOD = "emit";
  private static final String APPLET_BRIDGE_EVENT = "__APPLET_BRIDGE__";
  private static final String CALLBACK_TYPE = "callback";
  private JSONArray mFunctionCall;
  private JSONObject mCallbackData;
  private JSONArray mActionList;
  private JSONArray mJsbIgnoredInfo;
  private JSONObject mJsbSettings;
  private JSONObject mSharedData;

  public LynxRecorderReplayDataModule(LynxContext context) {
    super(context);
  }

  public LynxRecorderReplayDataModule(LynxContext context, Object param) {
    super(context, param);
    LynxRecorderReplayDataProvider provider = (LynxRecorderReplayDataProvider) param;
    mFunctionCall = provider.getFunctionCall();
    mCallbackData = provider.getCallbackData();
    mActionList = provider.getActionList();
    mJsbIgnoredInfo = provider.getJsbIgnoredInfo();
    mJsbSettings = provider.getJsbSettings();
    mSharedData = provider.getSharedData();
  }

  @LynxMethod
  public PiperData getSharedData(String key) {
    HashMap<String, Object> map = new HashMap<>();
    Object value = null;
    try {
      if (mSharedData != null) {
        value = mSharedData.get(key);
      }
    } catch (JSONException e) {
      LLog.e("LynxRecorder", "SharedData with key: " + key + " not found!");
    }
    map.put("value", value);
    PiperData data = PiperData.fromObject(map);
    return data;
  }

  @LynxMethod
  public void getData(final Callback callback) {
    JSONObject data = new JSONObject();
    try {
      data.put("RecordData", getRecordData());
      data.put("JsbIgnoredInfo", getJsbIgnoredInfo());
      data.put("JsbSettings", getJsbSettings());
    } catch (JSONException e) {
      LLog.e("TestBench", "Record file format error!");
      callback.invoke("{}");
    }
    callback.invoke(data.toString());
  }

  @LynxMethod
  public void emitReplayGlobalEvent(ReadableMap params) {
    if (mLynxContext == null || params == null) {
      return;
    }
    ReadableArray args = params.getArray("args", null);
    String recordedCallbackId = extractProtocolCallbackId(params);
    String runtimeCallbackId = extractProtocolCallbackId(args);
    String callbackId = recordedCallbackId != null ? recordedCallbackId : runtimeCallbackId;
    if (callbackId == null) {
      return;
    }
    try {
      JSONArray payload = findProtocolCallbackPayloads(callbackId);
      if (payload == null) {
        return;
      }
      JSONArray dispatchPayload = remapProtocolCallbackId(payload, runtimeCallbackId);
      mLynxContext.sendGlobalEvent(APPLET_BRIDGE_EVENT, jsonArrayToJavaOnlyArray(dispatchPayload));
    } catch (JSONException e) {
      LLog.e(TAG, "emitReplayGlobalEvent failed");
    }
  }

  private String getRecordData() {
    JSONObject json = new JSONObject();
    try {
      for (int index = 0; index < mFunctionCall.length(); index++) {
        JSONObject funcInvoke = mFunctionCall.getJSONObject(index);
        String moduleName = funcInvoke.getString("Module Name");
        if (!json.has(moduleName)) {
          json.put(moduleName, new JSONArray());
        }

        JSONObject methodLookUp = new JSONObject();

        String methodName = funcInvoke.getString("Method Name");

        long requestTime = Long.parseLong(funcInvoke.getString("Record Time")) * 1000;
        if (funcInvoke.has("RecordMillisecond")) {
          requestTime = funcInvoke.getLong("RecordMillisecond");
        }
        JSONObject params = funcInvoke.getJSONObject("Params");

        JSONArray callbackIDs;
        try {
          callbackIDs = params.getJSONArray("callback");
        } catch (JSONException e) {
          callbackIDs = null;
        }
        StringBuffer functionInvokeLabel = new StringBuffer();
        JSONArray callbackReturnValues = new JSONArray();
        if (callbackIDs != null) {
          for (int i = 0; i < callbackIDs.length(); i++) {
            String callbackId = callbackIDs.getString(i);
            JSONObject callbackInfo = getRecordedCallbackInfo(callbackId);
            if (callbackInfo != null) {
              callbackReturnValues.put(i, buildRecordedCallbackKernel(callbackInfo, requestTime));
            }
            functionInvokeLabel.append(callbackId).append("_");
          }
        } else {
          JSONObject protocolSyncAttributes =
              buildProtocolSyncAttributes(moduleName, methodName, params);
          if (protocolSyncAttributes != null) {
            methodLookUp.put("SyncAttributes", protocolSyncAttributes);
          }
        }

        if (!methodLookUp.has("SyncAttributes") && funcInvoke.has("SyncAttributes")) {
          methodLookUp.put("SyncAttributes", funcInvoke.getJSONObject("SyncAttributes"));
        }

        methodLookUp.put("Method Name", methodName);
        methodLookUp.put("Params", params);
        methodLookUp.put("Callback", callbackReturnValues);
        methodLookUp.put("Label", functionInvokeLabel.toString());

        json.getJSONArray(moduleName).put(methodLookUp);
      }
    } catch (JSONException e) {
      LLog.e("TestBench", "Record file format error!");
      return "{}";
    }
    return json.toString();
  }

  private String getJsbIgnoredInfo() {
    if (mJsbIgnoredInfo != null) {
      return mJsbIgnoredInfo.toString();
    } else {
      LLog.e("TestBench",
          "getJsbIgnoredInfo "
              + " error: download File failed");
      return "[]";
    }
  }

  private String getJsbSettings() {
    if (mJsbSettings != null) {
      return mJsbSettings.toString();
    } else {
      LLog.e("TestBench",
          "getJsbSettings "
              + " error: download File failed");
      return "{}";
    }
  }

  private JSONObject getRecordedCallbackInfo(String callbackId) {
    try {
      if (mCallbackData != null) {
        return mCallbackData.getJSONObject(callbackId);
      }
    } catch (JSONException e) {
      return null;
    }
    return null;
  }

  private JSONObject buildRecordedCallbackKernel(JSONObject callbackInfo, long requestTime)
      throws JSONException {
    long responseTime = Long.parseLong(callbackInfo.getString("Record Time")) * 1000;
    if (callbackInfo.has("RecordMillisecond")) {
      responseTime = callbackInfo.getLong("RecordMillisecond");
    }
    JSONObject callbackKernel = new JSONObject();
    callbackKernel.put("Value", callbackInfo.getJSONObject("Params"));
    callbackKernel.put("Delay", Math.max(0, responseTime - requestTime));
    return callbackKernel;
  }

  private JSONObject buildProtocolSyncAttributes(
      String moduleName, String methodName, JSONObject params) throws JSONException {
    if (!APPLET_BRIDGE_MODULE.equals(moduleName) || !POST_MESSAGE_METHOD.equals(methodName)) {
      return null;
    }
    String callbackId = extractProtocolCallbackId(params);
    if (callbackId == null) {
      return null;
    }
    JSONArray payload = findProtocolCallbackPayloads(callbackId);
    if (payload == null) {
      return null;
    }
    JSONObject syncAttributes = new JSONObject();
    syncAttributes.put("platformModule", "LynxRecorderReplayDataModule");
    syncAttributes.put("platformMethod", "emitReplayGlobalEvent");
    syncAttributes.put("label", callbackId + "_");
    return syncAttributes;
  }

  private String extractProtocolCallbackId(JSONObject params) {
    return extractProtocolCallbackId(params.optJSONArray("args"));
  }

  private String extractProtocolCallbackId(ReadableMap params) {
    if (params == null) {
      return null;
    }
    Object label = params.asHashMap().get("label");
    if (label == null) {
      return null;
    }
    return extractProtocolCallbackIdFromLabel(String.valueOf(label));
  }

  private String extractProtocolCallbackId(ReadableArray args) {
    if (args == null || args.size() == 0) {
      return null;
    }
    ReadableMap firstArg = args.getMap(0);
    if (firstArg == null || !firstArg.hasKey("callbackId")) {
      return null;
    }
    Object callbackId = firstArg.asHashMap().get("callbackId");
    return callbackId == null ? null : String.valueOf(callbackId);
  }

  private String extractProtocolCallbackId(JSONArray args) {
    if (args == null || args.length() == 0) {
      return null;
    }
    JSONObject firstArg = args.optJSONObject(0);
    if (firstArg == null || !firstArg.has("callbackId")) {
      return null;
    }
    return String.valueOf(firstArg.opt("callbackId"));
  }

  private String extractProtocolCallbackIdFromLabel(String label) {
    if (label == null || label.isEmpty() || "default".equals(label)) {
      return null;
    }
    int separatorIndex = label.indexOf('_');
    String callbackId = separatorIndex >= 0 ? label.substring(0, separatorIndex) : label;
    return callbackId.isEmpty() ? null : callbackId;
  }

  private JSONArray remapProtocolCallbackId(JSONArray payloads, String runtimeCallbackId)
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
      String recordedCallbackId = String.valueOf(payload.opt("callbackId"));
      if (runtimeCallbackId.equals(recordedCallbackId)) {
        continue;
      }
      try {
        long callbackIdAsLong = Long.parseLong(runtimeCallbackId);
        payload.put("callbackId", callbackIdAsLong);
      } catch (NumberFormatException longError) {
        try {
          double callbackIdAsDouble = Double.parseDouble(runtimeCallbackId);
          payload.put("callbackId", callbackIdAsDouble);
        } catch (NumberFormatException doubleError) {
          LLog.e(TAG,
              "remapProtocolCallbackId failed, runtimeCallbackId is not numeric: "
                  + runtimeCallbackId);
          continue;
        }
      }
    }
    return remapped;
  }

  private JSONArray findProtocolCallbackPayloads(String callbackId) throws JSONException {
    if (mActionList == null) {
      return null;
    }
    int candidateCount = 0;
    StringBuilder candidateCallbackIds = new StringBuilder();
    for (int index = 0; index < mActionList.length(); index++) {
      JSONObject action = mActionList.optJSONObject(index);
      if (action == null || !isAppletBridgeCallbackAction(action)) {
        continue;
      }
      JSONArray payloads = getAppletBridgeCallbackPayloads(action);
      if (payloads == null) {
        continue;
      }
      for (int payloadIndex = 0; payloadIndex < payloads.length(); payloadIndex++) {
        JSONObject payload = payloads.optJSONObject(payloadIndex);
        if (payload == null || !CALLBACK_TYPE.equals(payload.optString("type"))) {
          continue;
        }
        candidateCount++;
        if (candidateCount <= 10) {
          if (candidateCallbackIds.length() > 0) {
            candidateCallbackIds.append(',');
          }
          candidateCallbackIds.append(String.valueOf(payload.opt("callbackId")));
        }
        if (!callbackId.equals(String.valueOf(payload.opt("callbackId")))) {
          continue;
        }
        return new JSONArray().put(new JSONObject(payload.toString()));
      }
    }
    return null;
  }

  private boolean isAppletBridgeCallbackAction(JSONObject action) throws JSONException {
    if (!"sendGlobalEvent".equals(action.optString("Function Name"))) {
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

  private JSONArray getAppletBridgeCallbackPayloads(JSONObject action) throws JSONException {
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
        result.pushMap(LynxRecorderEventSend.jsonObjectToJavaOnlyMap((JSONObject) value));
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
}
