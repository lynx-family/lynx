// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.utils;

import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIShadowProxy;
import com.lynx.tasm.utils.CallStackUtil;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;

public class LynxUIMethodsExecutor {
  private static final Map<Class<?>, LynxUIMethodInvoker<?>> LYNX_UI_METHOD_INVOKER_MAP =
      new HashMap<>();

  public static void invokeMethod(
      LynxBaseUI ui, String methodName, ReadableMap params, Callback callback) {
    if (ui instanceof UIShadowProxy) {
      ui = ((UIShadowProxy) ui).getChild();
    }
    if (ui == null) {
      callback.invoke(LynxUIMethodConstants.NODE_NOT_FOUND, "node does not have a LynxUI");
      return;
    }
    LynxUIMethodInvoker methodInvoker = findLynxUIMethodInvoker(ui.getClass());
    try {
      methodInvoker.invoke(ui, methodName, params, callback);
    } catch (Exception e) {
      String errMsg = "Invoke method '" + methodName + "' error: " + e.getMessage();
      LynxError error =
          new LynxError(LynxSubErrorCode.E_EXCEPTION_PLATFORM, errMsg, "", LynxError.LEVEL_ERROR);
      error.setCallStack(CallStackUtil.getStackTraceStringWithLineTrimmed(e));
      error.setUserDefineInfo(ui.getPlatformCustomInfo());
      ui.getLynxContext().handleLynxError(error);
    }
  }

  public static void registerMethodInvoker(LynxUIMethodInvoker methodInvoker) {
    LYNX_UI_METHOD_INVOKER_MAP.put(methodInvoker.getClass(), methodInvoker);
  }

  static <T extends LynxBaseUI> LynxUIMethodInvoker<T> findLynxUIMethodInvoker(
      Class<? extends LynxBaseUI> cls) {
    LynxUIMethodInvoker<T> methodInvoker =
        (LynxUIMethodInvoker<T>) LYNX_UI_METHOD_INVOKER_MAP.get(cls);
    if (methodInvoker == null) {
      methodInvoker = findGeneratedMethodInvoker(cls);
      if (methodInvoker == null) {
        String log = "MethodInvoker not generated for class: " + cls.getName()
            + ". You should add module lynxProcessor";
        LLog.e("MethodsExecutor", log);

        if (LynxEnv.inst().isCheckPropsSetter() && LynxEnv.inst().isLynxDebugEnabled()) {
          throw new IllegalStateException(log);
        }

        methodInvoker = new LynxUIMethodsExecutor.FallbackLynxUIMethodInvoker<>(cls);
      }
      LYNX_UI_METHOD_INVOKER_MAP.put(cls, methodInvoker);
    }

    return methodInvoker;
  }

  private static <T> T findGeneratedMethodInvoker(Class<?> cls) {
    String clsName = cls.getName() + "$$MethodInvoker";
    try {
      Class<?> setterClass = Class.forName(clsName);
      return (T) setterClass.newInstance();
    } catch (ClassNotFoundException e) {
      return null;
    } catch (InstantiationException | IllegalAccessException e) {
      throw new RuntimeException("Unable to instantiate methods invoker for " + clsName, e);
    }
  }

  private static class FallbackLynxUIMethodInvoker<T extends LynxBaseUI>
      implements LynxUIMethodInvoker<T> {
    private Map<String, Method> mMethods;

    public FallbackLynxUIMethodInvoker(Class<? extends LynxBaseUI> cls) {
      mMethods = LynxUIMethodsCache.getNativeMethodsForLynxUIClass(cls);
    }

    @Override
    public void invoke(T ui, String methodName, ReadableMap params, Callback callback) {
      Method method = mMethods.get(methodName);
      if (method == null) {
        callback.invoke(LynxUIMethodConstants.METHOD_NOT_FOUND);
        return;
      }
      try {
        Class[] paramTypes = method.getParameterTypes();
        if (paramTypes.length == 0) {
          method.invoke(ui);
        } else if (paramTypes.length == 1) {
          if (paramTypes[0] == ReadableMap.class) {
            method.invoke(ui, params);
          } else if (paramTypes[0] == Callback.class) {
            method.invoke(ui, callback);
          }
        } else if (paramTypes[0] == ReadableMap.class && paramTypes[1] == Callback.class) {
          method.invoke(ui, params, callback);
        } else {
          callback.invoke(LynxUIMethodConstants.PARAM_INVALID);
          LLog.d("FallbackMethodInvoker", "invoke target method: params invalid");
        }
      } catch (Exception e) {
        callback.invoke(LynxUIMethodConstants.UNKNOWN);
        LLog.d("FallbackMethodInvoker", "invoke target method exception," + e.getMessage());
      }
    }
  }
}
