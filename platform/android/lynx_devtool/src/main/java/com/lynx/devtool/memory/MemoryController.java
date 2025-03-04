// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.memory;

import android.content.Context;
import com.lynx.debugrouter.DebugRouter;
import com.lynx.devtoolwrapper.MemoryListener;
import com.lynx.tasm.base.LLog;
import org.json.JSONException;
import org.json.JSONObject;

public class MemoryController implements MemoryListener.MemoryReporter {
  private final static String TAG = "MemoryController";
  private final static String TYPE = "CDP";
  private Context mContext;

  public static MemoryController getInstance() {
    return MemoryControllerLoader.INSTANCE;
  }

  private static class MemoryControllerLoader {
    private static final MemoryController INSTANCE = new MemoryController();
  }

  public void init(Context context) {
    mContext = context;
  }

  public void startMemoryTracing() {
    MemoryListener.getInstance().addMemoryReporter(this);
  }

  public void stopMemoryTracing() {
    MemoryListener.getInstance().removeMemoryReporter(this);
  }

  @Override
  public void uploadImageInfo(JSONObject data) {
    try {
      JSONObject msg = new JSONObject();
      JSONObject params = new JSONObject();
      msg.put("method", "Memory.uploadImageInfo");
      params.put("data", data);
      msg.put("params", params);
      DebugRouter.getInstance().sendDataAsync(TYPE, -1, msg.toString());
    } catch (JSONException e) {
      LLog.e(TAG, "Memory uploadImageInfo construct JSONObject failed:" + e.toString());
    }
  }
}
