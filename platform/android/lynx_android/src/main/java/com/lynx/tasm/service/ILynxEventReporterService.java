// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.NonNull;
import com.lynx.tasm.eventreport.ILynxEventReportObserver;

public interface ILynxEventReporterService extends IServiceProvider, ILynxEventReportObserver {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxEventReporterService.class;
  }
}
