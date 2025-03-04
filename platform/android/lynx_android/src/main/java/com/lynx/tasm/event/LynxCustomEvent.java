// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.event;

import java.util.HashMap;
import java.util.Map;

// TODO(hexionghui): Only LynxEvent, LynxCustomEvent, and LynxTouchEvent are retained.
public class LynxCustomEvent extends LynxEvent {
  protected Map<String, Object> mParams;

  public LynxCustomEvent(int tag, String type) {
    super(tag, type, LynxEventType.kCustom);
    mParams = new HashMap<>();
  }

  public LynxCustomEvent(int tag, String type, Map<String, Object> params) {
    super(tag, type, LynxEventType.kCustom);
    mParams = params;
  }

  public Map<String, Object> eventParams() {
    return mParams;
  }

  public void addDetail(String key, Object value) {
    if (mParams == null) {
      mParams = new HashMap<>();
    }
    mParams.put(key, value);
  }

  /**
   * On front-end, detail can be access by event.params.xx
   */
  public String paramsName() {
    // TODO(hexionghui): Change it to 'detail'.
    return "params";
  }
}
