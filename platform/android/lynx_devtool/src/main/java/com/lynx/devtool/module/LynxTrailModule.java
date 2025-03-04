// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.module;

import com.lynx.jsbridge.LynxContextModule;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.service.ILynxTrailService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;

/**
 * JSB for get lynx settings for devtool
 */
public class LynxTrailModule extends LynxContextModule {
  public static final String NAME = "LynxTrailModule";

  public LynxTrailModule(LynxContext context) {
    super(context);
  }

  /**
   * get settings from LynxTrailService
   */
  @LynxMethod
  public WritableMap getSettings() {
    ILynxTrailService service = LynxServiceCenter.inst().getService(ILynxTrailService.class);
    if (service == null) {
      return null;
    }
    Map<String, Object> settings = service.getAllValues();
    return (WritableMap) convertToJavaOnlyType(settings);
  }

  /**
   * convert Object to JavaOnlyMap/JavaOnlyArray for JSB
   */
  private Object convertToJavaOnlyType(Object value) {
    if (value instanceof Map) {
      Map<String, Object> valueMap = (Map<String, Object>) value;
      for (Map.Entry<String, Object> entry : valueMap.entrySet()) {
        entry.setValue(convertToJavaOnlyType(entry.getValue()));
      }
      return JavaOnlyMap.from(valueMap);
    }
    if (value instanceof List) {
      List<Object> valueList = (List<Object>) value;
      ListIterator<Object> iterator = valueList.listIterator();
      while (iterator.hasNext()) {
        Object iterValue = iterator.next();
        iterator.set(convertToJavaOnlyType(iterValue));
      }
      return JavaOnlyArray.from(valueList);
    }
    return value;
  }
}
