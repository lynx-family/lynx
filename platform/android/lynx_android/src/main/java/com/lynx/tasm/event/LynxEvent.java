// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.event;

import com.lynx.tasm.behavior.event.EventTargetBase;

public class LynxEvent {
  public static enum LynxEventType {
    kNone,
    kTouch,
    kMouse,
    kWheel,
    kKeyboard,
    kCustom,
  }
  private int mTag = -1;
  private String mName = "";
  private LynxEventType mType = LynxEventType.kNone;
  private EventTargetBase mTarget = null;
  private long mTimestamp = 0;

  public LynxEvent(int tag, String name, LynxEventType type) {
    mTag = tag;
    mName = name;
    mType = type;
    mTimestamp = System.currentTimeMillis();
  }

  public int getTag() {
    return mTag;
  }

  public String getName() {
    return mName;
  }

  public LynxEventType getType() {
    return mType;
  }

  public void setTarget(EventTargetBase target) {
    mTarget = target;
  }

  public EventTargetBase getTarget() {
    return mTarget;
  }

  public void setTimestamp(long timestamp) {
    mTimestamp = timestamp;
  }

  public long getTimestamp() {
    return mTimestamp;
  }
}
