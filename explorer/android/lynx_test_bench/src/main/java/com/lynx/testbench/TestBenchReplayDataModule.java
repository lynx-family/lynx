// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.testbench;

import android.content.Context;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.jsbridge.LynxModule;
import com.lynx.react.bridge.Callback;
import com.lynx.tasm.base.LLog;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class TestBenchReplayDataModule extends LynxModule {
  private static final String TAG = "TestBenchReplayDataModule";
  // "Invoked Method Data" field from record json file
  private static JSONArray mFunctionCall;
  // "Callback" field from record json file
  private static JSONObject mCallbackData;
  // replay additional info
  private static JSONArray mJsbIgnoredInfo;
  // "jsbSettings" field from record json file
  private static JSONObject mJsbSettings;

  private static long mReplayTime;

  public TestBenchReplayDataModule(Context context) {
    super(context);
  }

  public static void addFunctionCallArray(JSONArray responseData) {
    mFunctionCall = responseData;
  }

  public static void addCallbackDictionary(JSONObject callbackDictionary) {
    mCallbackData = callbackDictionary;
  }

  public static void setJsbIgnoredInfo(JSONArray jsbIgnoredInfo) {
    mJsbIgnoredInfo = jsbIgnoredInfo;
  }

  public static void setJsbSettings(JSONObject jsbSettings) {
    mJsbSettings = jsbSettings;
  }

  public static void setTime(long time) {
    mReplayTime = time;
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
              // get callback response Time
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
        }

        if (funcInvoke.has("SyncAttributes")) {
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

  public String replayTimeEnvJScript() {
    InputStream in = null;
    try {
      in = this.mContext.getAssets().open("testBench.js");
      int length = in.available();
      byte[] buffer = new byte[length];
      in.read(buffer);

      String script = new String(buffer);

      script = script.replace("###TESTBENCH_REPLAY_TIME###", String.valueOf(mReplayTime));

      File file = new File(this.mContext.getFilesDir(), "testBench.js");
      if (file.exists()) {
        if (!file.delete()) {
          LLog.e("TestBench", "Call replayTimeEnvJScript failed: file can't be deleted.");
          return "";
        }
      }
      if (file.createNewFile()) {
        String filePath = "";
        FileOutputStream outStream = null;
        try {
          outStream = new FileOutputStream(file);
          outStream.write(script.getBytes());
          filePath = "file://" + file.getPath();
        } catch (IOException e) {
          filePath = "";
          e.printStackTrace();
        } finally {
          if (outStream != null) {
            try {
              outStream.close();
            } catch (IOException e) {
              e.printStackTrace();
            }
          }
          return filePath;
        }
      } else {
        LLog.e("TestBench", "Call replayTimeEnvJScript failed: file can't created.");
        return "";
      }
    } catch (IOException e) {
      e.printStackTrace();
      return "";
    }
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
}
