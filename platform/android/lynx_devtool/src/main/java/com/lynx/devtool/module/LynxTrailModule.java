// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.module;

import com.lynx.jsbridge.LynxContextModule;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.service.ILynxTrailService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;

/**
 * JSB for get lynx settings for devtool
 */
public class LynxTrailModule extends LynxContextModule {
  public static final String NAME = "LynxTrailModule";
  private static final String GET_LATEST_SETTINGS_METHOD = "getLatestSettings";

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

  @LynxMethod
  public void getLatestSettings(final Callback resolve, final Callback reject) {
    final ILynxTrailService service = LynxServiceCenter.inst().getService(ILynxTrailService.class);
    if (service == null) {
      reject(reject, "Lynx Trail Service not registered");
      return;
    }
    try {
      Method method =
          service.getClass().getMethod(GET_LATEST_SETTINGS_METHOD, Callback.class, Callback.class);
      method.invoke(service,
          new Callback() {
            @Override
            public void invoke(Object... args) {
              Object settings = args != null && args.length > 0 ? args[0] : null;
              resolve(resolve, convertToJavaOnlyType(settings));
            }
          },
          new Callback() {
            @Override
            public void invoke(Object... args) {
              Object errorMessage = args != null && args.length > 0 ? args[0] : null;
              reject(reject, errorMessage instanceof String ? (String) errorMessage : null);
            }
          });
    } catch (NoSuchMethodException e) {
      reject(reject, "Lynx Trail Service does not support latest settings fetch");
    } catch (IllegalAccessException | InvocationTargetException e) {
      Throwable cause =
          e instanceof InvocationTargetException ? ((InvocationTargetException) e).getCause() : e;
      reject(reject,
          cause != null && cause.getMessage() != null ? cause.getMessage()
                                                      : "Fetch latest settings failed");
    }
  }

  private void resolve(Callback callback, Object result) {
    if (callback == null) {
      return;
    }
    if (mLynxContext == null) {
      callback.invoke(result);
      return;
    }
    mLynxContext.runOnJSThread(new Runnable() {
      @Override
      public void run() {
        callback.invoke(result);
      }
    });
  }

  private void reject(Callback callback, String message) {
    JavaOnlyMap error = new JavaOnlyMap();
    error.putString("message", message != null ? message : "Fetch latest settings failed");
    resolve(callback, error);
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
