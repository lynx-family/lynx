// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import android.content.Context;
import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import androidx.annotation.WorkerThread;
import org.json.JSONObject;

/**
 * Lynx Service Interface to report memory usage stats to available platforms
 */
public interface ILynxMemoryMonitorService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxMemoryMonitorService.class;
  }
  /**
   * Report Lynx memory usage with LynxMemoryInfo model
   *
   * @param data LynxMemoryInfo data object
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  @WorkerThread
  void reportMemoryUsage(@NonNull LynxMemoryInfo data);

  /**
   * Start memory allocation tracking with internal tool
   *
   * @param applicationContext App Context
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  @AnyThread
  void startTrackMemoryAllocation(@NonNull Context applicationContext);

  /**
   * Dump memory allocation reports and handle zip/upload logic for local memory allocation stacks
   *
   * @param isBaselineDump Whether current dump operation is for baseline metrics
   * @param payload        Dump operation context payload including session_id & template_url
   */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  @WorkerThread
  void dumpMemoryAllocationReport(boolean isBaselineDump, JSONObject payload);
}
