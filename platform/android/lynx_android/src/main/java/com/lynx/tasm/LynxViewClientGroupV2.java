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

  public void setInstanceId(int id) {
    mInstanceId = id;
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
    for (LynxViewClientV2 client : mClients) {
      client.onPageStarted(lynxView, info);
    }
  }

  @Override
  public void onPerformanceEvent(@NonNull PerformanceEntry entry) {
    for (LynxViewClientV2 client : mClients) {
      client.onPerformanceEvent(entry);
    }
  }

  @Override
  public void onResourceLoaded(@NonNull LynxResourceLoadInfo info) {
    for (LynxViewClientV2 client : mClients) {
      client.onResourceLoaded(info);
    }
  }
}
