// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.event;

import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import java.util.HashMap;
import java.util.Map;

public class EventsListener {
  public String name;
  public String type;
  public String functionName;
  public String lepusType;
  public String lepusFunctionName;

  private static final String NAME = "name";
  private static final String TYPE = "type";
  private static final String FUNCTION = "function";
  private static final String LEPUS_TYPE = "lepusType";
  private static final String LEPUS_FUNCTION = "lepusFunction";

  public EventsListener(
      String name, String type, String functionName, String lepusType, String lepusFunctionName) {
    this.name = name;
    this.type = type;
    this.functionName = functionName;
    this.lepusType = lepusType;
    this.lepusFunctionName = lepusFunctionName;
  }

  static public Map<String, EventsListener> convertEventListeners(ReadableArray events) {
    if (events == null) {
      return null;
    }
    Map<String, EventsListener> listenerMap = new HashMap<>();
    for (int i = 0; i < events.size(); i++) {
      ReadableMap event = events.getMap(i);
      String name = event.getString(NAME);
      EventsListener listener = listenerMap.get(name);
      if (listener == null) {
        listener = new EventsListener(name, event.getString(TYPE), event.getString(FUNCTION),
            event.getString(LEPUS_TYPE), event.getString(LEPUS_FUNCTION));
      } else {
        if (listener.type == null && listener.functionName == null) {
          listener.type = event.getString(TYPE);
          listener.functionName = event.getString(FUNCTION);
        }
        if (listener.lepusType == null && listener.lepusFunctionName == null) {
          listener.lepusType = event.getString(LEPUS_TYPE);
          listener.lepusFunctionName = event.getString(LEPUS_FUNCTION);
        }
      }
      listenerMap.put(listener.name, listener);
    }
    return listenerMap;
  }
}
