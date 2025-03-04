// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.event;

import java.util.HashMap;
import java.util.Map;

/**
 * @deprecated Use {@link LynxCustomEvent} instead.
 */
@Deprecated
public class LynxDetailEvent extends LynxCustomEvent {
  public LynxDetailEvent(int tag, String type) {
    super(tag, type);
  }

  public LynxDetailEvent(int tag, String type, Map<String, Object> details) {
    super(tag, type, details);
  }

  @Override
  public Map<String, Object> eventParams() {
    return mParams;
  }

  /**
   * On front-end, detail can be access by event.detail.xx
   */
  @Override
  public String paramsName() {
    return "detail";
  }
}
