// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.lepus;

import androidx.annotation.NonNull;
import com.lynx.jsbridge.LynxModuleFactory;
import com.lynx.jsbridge.LynxModuleWrapper;
import com.lynx.jsbridge.MethodDescriptor;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.common.LepusBuffer;
import com.lynx.tasm.core.LynxEngineProxy;
import com.lynx.tasm.core.LynxThreadPool;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

public class LynxLepusModule {
  private static final String TAG = "LynxLepusModule";

  private static final String PARAM_METHOD_DETAIL = "methodDetail";
  private static final String PARAM_MODULE = "module";
  private static final String PARAM_METHOD = "method";
  private static final String PARAM_DATA = "param";
  private static final String PARAM_ENTRY = "tasmEntryName";
  private static final String PARAM_CALLBACKID = "callbackId";
  private static final String METHOD_CALL = "call";
  private static final String METHOD_CALL_SYNC = "callSync";
  private static final String PARAM_FROM_PIPER = "fromPiper";

  public static void triggerLepusBridgeAsync(String methodName, Object args,
      final LynxEngineProxy actor, final LynxModuleFactory mModuleFactory) {
    if (!(args instanceof ReadableMap)) {
      return;
    }
    if (mModuleFactory == null) {
      LLog.e(TAG, "mModuleFactory does not exist!");
      return;
    }
    ReadableMap argsMap = (ReadableMap) args;
    ReadableMap methodDetailMap = argsMap.getMap(PARAM_METHOD_DETAIL, new JavaOnlyMap());
    String moduleName = methodDetailMap.getString(PARAM_MODULE, null);
    String method = methodDetailMap.getString(PARAM_METHOD, null);

    if (moduleName == null) {
      return;
    }
    LynxModuleWrapper moduleWrapper = mModuleFactory.getModule(moduleName);
    if (moduleWrapper == null) {
      moduleWrapper = LynxEnv.inst().getModuleFactory().getModule(moduleName);
    }
    if (moduleWrapper == null) {
      return;
    }

    boolean useAirThread = !((ReadableMap) args).getBoolean("lepusUseUIThread", false)
        && ((ReadableMap) args).getBoolean("useAirThread", false);
    // TODO(zhangqun.29): We will discuss with DSL later to delete useAirThread and all will be
    // controlled by lepusUseUIThread.
    if (useAirThread) {
      WeakReference<LynxModuleWrapper> lynxModuleWrapperWeakRef =
          new WeakReference<>(moduleWrapper);
      WeakReference<LynxEngineProxy> actorWeakRef = new WeakReference<>(actor);
      LynxThreadPool.getAsyncLepusBridgeExecutor().execute(new Runnable() {
        @Override
        public void run() {
          LynxModuleWrapper wrapper = lynxModuleWrapperWeakRef.get();
          LynxEngineProxy actor = actorWeakRef.get();
          if (wrapper != null && actor != null) {
            invokeLepusModuleInternal(wrapper, actor, argsMap, methodName);
          } else {
            LLog.i(
                TAG, "triggerLepusBridgeAsync failed, LynxModule or LynxEngine has been released.");
          }
        }
      });
    } else {
      invokeLepusModuleInternal(moduleWrapper, actor, argsMap, methodName);
    }
    return;
  }

  private static void invokeLepusModuleInternal(@NonNull LynxModuleWrapper moduleWrapper,
      LynxEngineProxy actor, @NonNull ReadableMap argsMap, String methodName) {
    ReadableMap methodDetailMap = argsMap.getMap(PARAM_METHOD_DETAIL, new JavaOnlyMap());
    String moduleName = methodDetailMap.getString(PARAM_MODULE, null);
    String method = methodDetailMap.getString(PARAM_METHOD, null);
    final boolean fromPiper = argsMap.getBoolean(PARAM_FROM_PIPER, false);
    final String entryName = argsMap.getString(PARAM_ENTRY, null);
    final int callbackID = argsMap.getInt(PARAM_CALLBACKID, -1);

    // in case of call method, we need to insert method value to first arg
    boolean isCallMethod =
        methodName.equals(METHOD_CALL) || (method != null && method.length() != 0);
    try {
      for (MethodDescriptor methodDescriptor : moduleWrapper.getMethodDescriptors()) {
        String name = methodDescriptor.getName();
        if (name == null || name.isEmpty() || !name.equals(methodName)) {
          continue;
        }
        WeakReference<LynxEngineProxy> actorWeakRef = new WeakReference<>(actor);
        if (isCallMethod) {
          // string, map, callback
          methodDescriptor.getMethod().invoke(moduleWrapper.getModule(), method, methodDetailMap,
              new com.lynx.react.bridge.Callback() {
                @Override
                public void invoke(Object... argus) {
                  // send to return
                  // Methods from piper do not require callbacks
                  LynxEngineProxy actor = actorWeakRef.get();
                  if (actor == null) {
                    LLog.i(TAG, "triggerLepusBridgeAsync failed, LynxEngine has been released.");
                    return;
                  }
                  if (argus.length > 0 && !fromPiper) {
                    ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(argus[0]);
                    actor.invokeLepusApiCallback(callbackID, entryName, buffer);
                  }
                }
              });
        } else {
          if (fromPiper) {
            final ReadableArray paramArray =
                methodDetailMap.getArray(PARAM_DATA, new JavaOnlyArray());
            methodDescriptor.getMethod().invoke(
                moduleWrapper.getModule(), paramArray.asArrayList().toArray());
          } else {
            // map, callback
            methodDescriptor.getMethod().invoke(
                moduleWrapper.getModule(), methodDetailMap, new com.lynx.react.bridge.Callback() {
                  @Override
                  public void invoke(Object... argus) {
                    LynxEngineProxy actor = actorWeakRef.get();
                    if (actor == null) {
                      LLog.i(TAG, "triggerLepusBridgeAsync failed, LynxEngine has been released.");
                      return;
                    }
                    // send to return
                    if (argus.length > 0) {
                      ByteBuffer buffer = LepusBuffer.INSTANCE.encodeMessage(argus[0]);
                      actor.invokeLepusApiCallback(callbackID, entryName, buffer);
                    }
                  }
                });
          }
        }
      }
    } catch (Exception e) {
      LLog.e(TAG, "Lepus Module invokeAsync: " + e.toString());
    }
  }

  public static Object triggerLepusBridge(
      String methodName, Object args, final LynxModuleFactory mModuleFactory) {
    if (!(args instanceof ReadableMap)) {
      return null;
    }
    if (mModuleFactory == null) {
      LLog.e(TAG, "mModuleFactory does not exist!");
      return null;
    }
    ReadableMap argsMap = (ReadableMap) args;
    ReadableMap methodDetailMap = argsMap.getMap(PARAM_METHOD_DETAIL, new JavaOnlyMap());
    String moduleName = methodDetailMap.getString(PARAM_MODULE, null);
    String method = methodDetailMap.getString(PARAM_METHOD, null);
    if (moduleName == null) {
      return null;
    }
    LynxModuleWrapper moduleWrapper = mModuleFactory.getModule(moduleName);
    if (moduleWrapper == null) {
      moduleWrapper = LynxEnv.inst().getModuleFactory().getModule(moduleName);
    }
    if (moduleWrapper == null) {
      return null;
    }
    boolean isCallMethod = methodName.equals(METHOD_CALL) || methodName.equals(METHOD_CALL_SYNC)
        || (method != null && method.length() != 0);
    for (MethodDescriptor methodDescriptor : moduleWrapper.getMethodDescriptors()) {
      String name = methodDescriptor.getName();
      if (name == null || name.isEmpty()) {
        continue;
      }
      // process sync method
      if (name.equals(methodName)) {
        try {
          if (isCallMethod) {
            // method, map
            return methodDescriptor.getMethod().invoke(
                moduleWrapper.getModule(), method, methodDetailMap);
          } else {
            // map
            return methodDescriptor.getMethod().invoke(moduleWrapper.getModule(), methodDetailMap);
          }
        } catch (Exception e) {
          LLog.e(TAG, "Lepus Module invokeSync: " + e.toString());
        }
      }
    }
    return null;
  }
}
