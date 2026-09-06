// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.recorder;

import android.os.SystemClock;
import android.view.KeyEvent;
import android.view.MotionEvent;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxView;
import java.util.Iterator;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class LynxRecorderEventSend {
  private static MotionEvent getMotionEvent(JSONObject params, float xScaling, float yScaling) {
    try {
      int action = Integer.parseInt(params.getString("action"));
      float x = Float.parseFloat(params.getString("x")) * xScaling;
      float y = Float.parseFloat(params.getString("y")) * yScaling;
      int metaState = Integer.parseInt(params.getString("metaState"));
      return MotionEvent.obtain(
          SystemClock.uptimeMillis(), SystemClock.uptimeMillis(), action, x, y, metaState);
    } catch (JSONException e) {
      e.printStackTrace();
      return null;
    }
  }

  private static KeyEvent getKeyEvent(JSONObject params) {
    try {
      int action = Integer.parseInt(params.getString("action"));
      int keycode = Integer.parseInt(params.getString("keycode"));
      return new KeyEvent(action, keycode);
    } catch (JSONException e) {
      e.printStackTrace();
      return null;
    }
  }

  private static JavaOnlyMap jsonObjectToJavaOnlyMap(JSONObject obj) {
    Iterator iterator = obj.keys();
    JavaOnlyMap result = new JavaOnlyMap();
    while (iterator.hasNext()) {
      String key = iterator.next().toString();
      Object value = obj.opt(key);
      if (isJsonNull(value)) {
        continue;
      }
      if (value instanceof JSONArray) {
        result.put(key, jsonArrayToJavaOnlyArray((JSONArray) value));
      } else if (value instanceof JSONObject) {
        result.put(key, jsonObjectToJavaOnlyMap((JSONObject) value));
      } else {
        result.put(key, value);
      }
    }
    return result;
  }

  private static JavaOnlyArray jsonArrayToJavaOnlyArray(JSONArray array) {
    JavaOnlyArray result = new JavaOnlyArray();
    for (int index = 0; index < array.length(); index++) {
      Object value = array.opt(index);
      if (isJsonNull(value)) {
        continue;
      }
      if (value instanceof JSONArray) {
        result.add(jsonArrayToJavaOnlyArray((JSONArray) value));
      } else if (value instanceof JSONObject) {
        result.add(jsonObjectToJavaOnlyMap((JSONObject) value));
      } else {
        result.add(value);
      }
    }
    return result;
  }

  private static boolean isJsonNull(Object value) {
    return value == null || value == JSONObject.NULL;
  }

  // send tap/longtap/drag.etc native gesture event
  public static void sendEventAndroid(
      JSONObject params, LynxView view, float xScaling, float yScaling) {
    String type = null;
    try {
      type = params.getString("type");
    } catch (JSONException ignored) {
      // The old testbench does not have this key.
    }

    if (type != null && type.equals("key")) {
      view.dispatchKeyEvent(getKeyEvent(params));
    } else {
      view.dispatchTouchEvent(getMotionEvent(params, xScaling, yScaling));
    }
  }

  // send GlobalEvent from native code to js
  public static void sendGlobalEvent(JSONObject params, LynxView view) {
    try {
      JSONArray arguments = params.getJSONArray("arguments");
      if (arguments.length() != 2) {
        return;
      }
      // first is eventName
      String name = arguments.getString(0);
      if (name.equals("exposure") || name.equals("disexposure")) {
        return;
      }
      // second is event infomation
      JSONArray args = arguments.getJSONArray(1);

      if (view != null) {
        view.sendGlobalEvent(name, jsonArrayToJavaOnlyArray(args));
      }
    } catch (JSONException e) {
      e.printStackTrace();
    }
  }
}
