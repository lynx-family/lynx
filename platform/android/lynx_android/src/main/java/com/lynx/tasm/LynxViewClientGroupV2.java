// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.eventreport.LynxEventReporter;
import com.lynx.tasm.performance.performanceobserver.PerformanceEntry;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;

public class LynxViewClientGroupV2 extends LynxViewClientV2 {
  private CopyOnWriteArrayList<LynxViewClientV2> mClients = new CopyOnWriteArrayList<>();
  private int mInstanceId = LynxEventReporter.INSTANCE_ID_UNKNOWN;
  private boolean enableLifecycleTimeReport = false;

  public void setInstanceId(int id) {
    mInstanceId = id;
  }

  public void setEnableLifecycleTimeReport(boolean enabled) {
    enableLifecycleTimeReport = enabled;
  }

  // Some callbacks are very frequent, so we need to directly record the start time as a parameter
  // during execution, rather than encapsulating the entire execution content into a Runnable for
  // cleaner code style.
  private void recordLifecycleTimeWithStartTime(String callbackName, long startTime) {
    if (mInstanceId == LynxEventReporter.INSTANCE_ID_UNKNOWN) {
      return;
    }

    long overallEndTime = System.nanoTime();
    double overallMillsDuration = (overallEndTime - startTime) / 1000000d;

    String threadName = Thread.currentThread().getName();
    LynxEventReporter.PropsBuilder builder = () -> {
      Map<String, Object> props = new HashMap<>();
      props.put("name", callbackName);
      props.put("duration", overallMillsDuration);
      props.put("thread", threadName);
      return props;
    };
    LynxEventReporter.onEvent("lynxsdk_lifecycle_time", mInstanceId, builder);
  }

  public void addClient(LynxViewClientV2 client) {
    if (client != null && !mClients.contains(client)) {
      mClients.add(client);
    }
  }

  public void removeClient(LynxViewClientV2 client) {
    mClients.remove(client);
  }

  @Override
  public void onPageStarted(@Nullable LynxView lynxView, @NonNull LynxPipelineInfo info) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClientV2 client : mClients) {
      client.onPageStarted(lynxView, info);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onPageStarted", startTime);
    }
  }

  @Override
  public void onPerformanceEvent(@NonNull PerformanceEntry entry) {
    long startTime = -1;
    if (enableLifecycleTimeReport) {
      startTime = System.nanoTime();
    }
    for (LynxViewClientV2 client : mClients) {
      client.onPerformanceEvent(entry);
    }
    if (startTime != -1) {
      recordLifecycleTimeWithStartTime("onPerformanceEvent", startTime);
    }
  }
}
