// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.event;

import java.util.HashMap;
import java.util.Map;
import org.json.JSONException;
import org.json.JSONObject;

public class LynxImpressionEvent extends LynxCustomEvent {
  public static final String EVENT_ATTACH = "attach";
  public static final String EVENT_DETACH = "detach";

  private String mParamsName;
  private HashMap<String, Object> mParams;

  public LynxImpressionEvent(int tag, String type) {
    super(tag, type);
  }

  public static LynxImpressionEvent createAttachEvent(int tag) {
    return new LynxImpressionEvent(tag, EVENT_ATTACH);
  }

  public static LynxImpressionEvent createDetachEvent(int tag) {
    return new LynxImpressionEvent(tag, EVENT_DETACH);
  }

  public static LynxImpressionEvent createImpressionEvent(int tag, String type) {
    return new LynxImpressionEvent(tag, type);
  }

  public void setParmas(String key, HashMap<String, Object> params) {
    mParamsName = key;
    mParams = params;
  }

  @Override
  public Map<String, Object> eventParams() {
    if (mParams == null) {
      return new HashMap<>();
    }
    return mParams;
  }

  public String paramsName() {
    if (mParamsName != null) {
      return mParamsName;
    }
    return "params";
  }
}
