// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import android.content.Context;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.jsbridge.LynxModule;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.PiperData;
import com.lynx.tasm.base.LLog;
import java.math.BigDecimal;
import java.util.HashMap;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class LynxRecorderReplayDataModule extends LynxModule {
  private static final String TAG = "LynxRecorderReplayDataModule";
  private static final String APPLET_BRIDGE_MODULE = "AppletBridgeModule";
  private static final String POST_MESSAGE_METHOD = "postMessage";
  private static final String GLOBAL_EVENT_EMITTER = "GlobalEventEmitter";
  private static final String EMIT_METHOD = "emit";
  private static final String APPLET_BRIDGE_EVENT = "__APPLET_BRIDGE__";
  private static final String CALLBACK_TYPE = "callback";
  private static final String PLATFORM_MODULE = "LynxRecorderReplaySyncModule";
  private static final String PLATFORM_METHOD = "emitReplayGlobalEvent";

  private JSONArray mFunctionCall;
  private JSONObject mCallbackData;
  private JSONArray mActionList;
  private JSONArray mJsbIgnoredInfo;
  private JSONObject mJsbSettings;
  private JSONObject mSharedData;

  public LynxRecorderReplayDataModule(Context context) {
    super(context);
  }

  public LynxRecorderReplayDataModule(Context context, Object param) {
    super(context);
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
    return PiperData.fromObject(map);
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
            JSONObject callbackInfo;
            try {
              callbackInfo = mCallbackData.getJSONObject(callbackIDs.getString(i));
            } catch (JSONException e) {
              callbackInfo = null;
            }
            if (callbackInfo != null) {
              long responseTime = Long.parseLong(callbackInfo.getString("Record Time")) * 1000;
              if (funcInvoke.has("RecordMillisecond")) {
                responseTime = callbackInfo.getLong("RecordMillisecond");
              }
              JSONObject callbackKernel = new JSONObject();
              callbackKernel.put("Value", callbackInfo.getJSONObject("Params"));
              callbackKernel.put("Delay", responseTime - requestTime);
              callbackReturnValues.put(i, callbackKernel);
            }
            functionInvokeLabel.append(callbackIDs.getString(i)).append("_");
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

  private JSONObject buildProtocolSyncAttributes(
      String moduleName, String methodName, JSONObject params) throws JSONException {
    if (!APPLET_BRIDGE_MODULE.equals(moduleName) || !POST_MESSAGE_METHOD.equals(methodName)) {
      return null;
    }
    String callbackId = extractProtocolCallbackId(params.optJSONArray("args"));
    if (callbackId == null || findProtocolCallbackPayloads(callbackId) == null) {
      return null;
    }
    JSONObject syncAttributes = new JSONObject();
    syncAttributes.put("platformModule", PLATFORM_MODULE);
    syncAttributes.put("platformMethod", PLATFORM_METHOD);
    syncAttributes.put("label", callbackId + "_");
    return syncAttributes;
  }

  private String extractProtocolCallbackId(JSONArray args) {
    if (args == null || args.length() == 0) {
      return null;
    }
    JSONObject firstArg = args.optJSONObject(0);
    if (firstArg == null || !firstArg.has("callbackId")) {
      return null;
    }
    return normalizeCallbackId(firstArg.opt("callbackId"));
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

  private JSONArray findProtocolCallbackPayloads(String callbackId) {
    if (mActionList == null) {
      return null;
    }
    for (int index = 0; index < mActionList.length(); index++) {
      JSONObject action = mActionList.optJSONObject(index);
      if (!isAppletBridgeCallbackAction(action)) {
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
        if (isSameCallbackId(callbackId, payload.opt("callbackId"))) {
          return payloads;
        }
      }
    }
    return null;
  }

  private boolean isAppletBridgeCallbackAction(JSONObject action) {
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

  private JSONArray getAppletBridgeCallbackPayloads(JSONObject action) {
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
}
