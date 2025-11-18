// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import com.lynx.jsbridge.LynxEmbeddedModule;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.group.ILynxViewGroup;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

public class DefaultLogicExecutor implements ILynxLogicExecutor {
  private static final String TAG = "DefaultLogicExecutor";
  // event related
  public static final String EVENT_METHOD = "method";
  public static final String EVENT_ARGS = "args";
  private static final String MODULE_NAME = "embeddedModule";

  // sendGlobalEvent related event
  public static final String GLOBAL_EVENT_METHOD = "sendGlobalEvent";
  public static final String GLOBAL_EVENT_NAME = "name";
  public static final String GLOBAL_EVENT_PARAMS = "params";

  // runtime related
  private static final String LOGIC_JS_PATH = "/logic.js";
  private static final String APP_SERVICE_PATH = "/app-service.js";

  // Lifecycle
  public static final String LIFECYCLE_EVENT_ON_LOAD = "onLoad";
  public static final String LIFECYCLE_EVENT_ON_DATA_CHANGED = "onDataChanged";
  public static final String LIFECYCLE_EVENT_ON_DESTROY = "onDestroy";

  private final Object mInitLock = new Object();
  private volatile LynxBackgroundRuntime mRuntime;
  private WeakReference<ILynxViewGroup> mLynxViewGroupRef;

  public DefaultLogicExecutor() {
    mLynxViewGroupRef = new WeakReference<ILynxViewGroup>(null);
  }

  public DefaultLogicExecutor(ILynxViewGroup lynxViewGroup) {
    mLynxViewGroupRef = new WeakReference<>(lynxViewGroup);
  }

  public void init(ILynxViewGroup viewGroup) {
    mLynxViewGroupRef = new WeakReference<>(viewGroup);
  }

  private void initLynxBackgroundRuntimeIfNeeded(LynxContext lynxContext) {
    if (mRuntime == null) {
      synchronized (mInitLock) {
        if (mRuntime == null) {
          TraceEvent.beginSection(TraceEventDef.LOGIC_EXECUTOR_INIT);
          final ILynxViewGroup viewGroup = mLynxViewGroupRef.get();

          if (viewGroup == null) {
            LLog.e(TAG, "LynxViewGroup not found");
            return;
          }
          final LynxBackgroundRuntimeOptions options = viewGroup.getLynxRuntimeOptions();
          final TemplateBundle bundle = viewGroup.getTemplateBundleNonBlocking();
          if (options == null || bundle == null || lynxContext == null) {
            LLog.e(TAG, "init LynxBackgroundRuntime failed.");
            return;
          }
          options.registerModule(LynxEmbeddedModule.NAME, LynxEmbeddedModule.class, viewGroup);
          mRuntime = new LynxBackgroundRuntime(lynxContext, options, viewGroup.isDebuggable());
          String url = LOGIC_JS_PATH;
          String bundleUrl = bundle.getUrl();
          if (bundleUrl != null) {
            if (bundleUrl.startsWith("/") || bundleUrl.startsWith("http")) {
              url = bundleUrl + url;
            }
          }
          mRuntime.evaluateTemplateBundle(url, bundle, APP_SERVICE_PATH);
          TraceEvent.endSection(TraceEventDef.LOGIC_EXECUTOR_INIT);
        }
      }
    }
  }

  @Override
  public void onLynxEvent(LynxView lynxView, ReadableMap event) {
    if (!event.hasKey(EVENT_METHOD)) {
      return;
    }
    initLynxBackgroundRuntimeIfNeeded(lynxView.getLynxContext());
    if (mRuntime == null) {
      return;
    }
    processEvent(lynxView, event);
  }

  private void processEvent(LynxView lynxView, ReadableMap event) {
    String method_name = event.getString(EVENT_METHOD);
    if (TraceEvent.isTracingStarted()) {
      Map<String, String> map = new HashMap<>();
      map.put("name", method_name);
      TraceEvent.beginSection(TraceEventDef.LOGIC_EXECUTOR_EVENT, map);
    }
    if (method_name == GLOBAL_EVENT_METHOD) {
      String name = event.getString(GLOBAL_EVENT_NAME);
      JavaOnlyArray params = (JavaOnlyArray) event.getArray(GLOBAL_EVENT_PARAMS);
      params.pushInt(lynxView.getLynxViewId());
      mRuntime.sendGlobalEvent(name, params);
    } else {
      JavaOnlyArray args = new JavaOnlyArray();
      if (event.getType(EVENT_ARGS) == ReadableType.TemplateData) {
        TemplateData args_data = (TemplateData) (event.getTemplateData(EVENT_ARGS));
        args.pushTemplateData(args_data);
      } else if (event.getType(EVENT_ARGS) == ReadableType.Map) {
        JavaOnlyMap args_map = (JavaOnlyMap) (event.getMap(EVENT_ARGS));
        args.pushMap(args_map);
      }
      args.pushInt(lynxView.getLynxViewId());
      mRuntime.callFunction(MODULE_NAME, method_name, args);
    }
    if (TraceEvent.isTracingStarted()) {
      TraceEvent.endSection(TraceEventDef.LOGIC_EXECUTOR_EVENT);
    }
  }

  @Override
  public void destroy() {
    if (mRuntime != null) {
      mRuntime.destroy();
      mRuntime = null;
    }
  }
}
