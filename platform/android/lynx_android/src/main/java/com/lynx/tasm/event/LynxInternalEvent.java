// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.event;

import java.util.Map;

public class LynxInternalEvent {
  public static final int NEED_VALIDATE = 0x0;

  private int mTag;

  private int mInternalEventId;

  public LynxInternalEvent(int tag, int id) {
    mTag = tag;
    mInternalEventId = id;
  }

  public int getTag() {
    return mTag;
  }

  public int getEventId() {
    return mInternalEventId;
  }

  public Map<String, Object> getParams() {
    return null;
  }
}
