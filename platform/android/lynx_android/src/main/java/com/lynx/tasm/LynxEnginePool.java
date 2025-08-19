// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;

class LynxEnginePool {
  private static final String TAG = "LynxEnginePool";

  private Map<TemplateBundle, LinkedList<LynxEngine>> mCache = new HashMap<>();

  private static class Holder {
    private static final LynxEnginePool INSTANCE = new LynxEnginePool();
  }

  static LynxEnginePool getInstance() {
    return Holder.INSTANCE;
  }

  void registerReuseEngineWrapper(LynxEngine engineWrapper) {
    if (engineWrapper == null) {
      return;
    }
    LinkedList<LynxEngine> engineQueue = getEngineQueue(engineWrapper.getTemplateBundle());
    if (TraceEvent.isTracingStarted()) {
      Map<String, String> map = new HashMap<>();
      map.put("engineQueue", engineQueue.toString());
      map.put("registerEngine", String.valueOf(engineWrapper));
      map.put("templateBundle", String.valueOf(engineWrapper.getTemplateBundle()));
      TraceEvent.beginSection(TraceEventDef.LYNX_ENGINE_POOL_REGISTER_ENGINE, map);
    }
    synchronized (this) {
      if (!engineQueue.contains(engineWrapper)) {
        engineQueue.offer(engineWrapper);
      }
    }
    engineWrapper.setQueueRefFromPool(engineQueue);
    LLog.i(TAG,
        "registerReuseEngineWrapper EngineQueue Cache: " + engineQueue
            + ", bundle:" + engineWrapper.getTemplateBundle());
    TraceEvent.endSection(TraceEventDef.LYNX_ENGINE_POOL_REGISTER_ENGINE);
  }

  @Nullable
  LynxEngine pollEngineFromPool(
      TemplateBundle templateBundle, ThreadStrategyForRendering threadStrategyForRendering) {
    LinkedList<LynxEngine> engineQueue = getEngineQueue(templateBundle);
    LLog.i(TAG, "pollEngine EngineQueue Cache: " + engineQueue + ", bundle:" + templateBundle);
    if (TraceEvent.isTracingStarted()) {
      Map<String, String> map = new HashMap<>();
      map.put("engineQueue", engineQueue.toString());
      map.put("templateBundle", String.valueOf(templateBundle));
      TraceEvent.beginSection(TraceEventDef.LYNX_ENGINE_POOL_POOL_ENGINE, map);
    }
    LynxEngine backupLynxEngine = null;
    LynxEngine result = null;
    synchronized (this) {
      for (int i = 0; i < engineQueue.size(); i++) {
        LynxEngine lynxEngine = engineQueue.get(i);
        // Prioritize fetching engines with the same thread mode
        if (threadStrategyForRendering == lynxEngine.getThreadStrategy()
            && lynxEngine.tryBeReusing()) {
          engineQueue.remove(i);
          result = lynxEngine;
          TraceEvent.endSection("pollEngineFromPool");
          break;
        } else if (lynxEngine.canReused()) {
          backupLynxEngine = lynxEngine;
        }
      }
      if (result == null && backupLynxEngine != null && backupLynxEngine.tryBeReusing()) {
        result = backupLynxEngine;
      }
    }
    TraceEvent.endSection(TraceEventDef.LYNX_ENGINE_POOL_POOL_ENGINE);
    return result;
  }

  void delete(@NonNull LynxEngine lynxEngine) {
    TemplateBundle templateBundle = lynxEngine.getTemplateBundle();
    if (templateBundle == null) {
      return;
    }
    LinkedList<LynxEngine> engineQueue = getEngineQueue(templateBundle);
    LLog.i(TAG, "deleteEngine EngineQueue Cache: " + engineQueue + ", bundle:" + templateBundle);
    synchronized (this) {
      engineQueue.remove(lynxEngine);
    }
  }

  @NonNull
  private synchronized LinkedList<LynxEngine> getEngineQueue(TemplateBundle templateBundle) {
    LinkedList<LynxEngine> engineQueue = mCache.get(templateBundle);
    if (engineQueue == null) {
      engineQueue = new LinkedList<>();
      mCache.put(templateBundle, engineQueue);
      LinkedList<LynxEngine> finalEngineQueue = engineQueue;
      templateBundle.setOnReleaseCallback(new TemplateBundle.OnReleaseCallback() {
        @Override
        public void onRelease() {
          synchronized (LynxEnginePool.this) {
            finalEngineQueue.clear();
            mCache.remove(templateBundle);
          }
        }
      });
    }
    return engineQueue;
  }
}
