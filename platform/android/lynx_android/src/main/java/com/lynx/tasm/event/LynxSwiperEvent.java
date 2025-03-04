// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.event;

public class LynxSwiperEvent extends LynxDetailEvent {
  public static final String EVENT_CHANGE = "change";

  public LynxSwiperEvent(int tag, String type) {
    super(tag, type);
  }

  public static LynxSwiperEvent createSwiperEvent(int tag, String type) {
    return new LynxSwiperEvent(tag, type);
  }

  public void setScrollParmas(int current) {
    addDetail("current", current);
  }
}
