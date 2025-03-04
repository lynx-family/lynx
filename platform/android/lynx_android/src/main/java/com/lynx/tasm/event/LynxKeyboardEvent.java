// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.event;

import java.util.Map;

public class LynxKeyboardEvent extends LynxCustomEvent {
  public static final String KEYBOARD_STATUS_CHANGED = "keyboardstatuschanged";

  public LynxKeyboardEvent(int tag, String type, Map<String, Object> params) {
    super(tag, type, params);
  }
}
