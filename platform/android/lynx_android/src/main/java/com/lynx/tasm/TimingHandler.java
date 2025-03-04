// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import java.util.HashMap;
import java.util.Map;

public class TimingHandler {
  public static final String CREATE_LYNX_START = "createLynxStart";
  public static final String CREATE_LYNX_END = "createLynxEnd";

  public static final String OPEN_TIME = "openTime";
  public static final String CONTAINER_INIT_START = "containerInitStart";
  public static final String CONTAINER_INIT_END = "containerInitEnd";
  public static final String PREPARE_TEMPLATE_START = "prepareTemplateStart";
  public static final String PREPARE_TEMPLATE_END = "prepareTemplateEnd";

  public static class ExtraTimingInfo {
    public long mOpenTime = 0;
    public long mContainerInitStart = 0;
    public long mContainerInitEnd = 0;
    public long mPrepareTemplateStart = 0;
    public long mPrepareTemplateEnd = 0;

    public Map<String, Long> toMap() {
      HashMap<String, Long> map = new HashMap<>();
      map.put(OPEN_TIME, mOpenTime);
      map.put(CONTAINER_INIT_START, mContainerInitStart);
      map.put(CONTAINER_INIT_END, mContainerInitEnd);
      map.put(PREPARE_TEMPLATE_START, mPrepareTemplateStart);
      map.put(PREPARE_TEMPLATE_END, mPrepareTemplateEnd);
      return map;
    }
  }
}
