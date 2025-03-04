// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import androidx.annotation.NonNull;

/*
 * The selection of lynx log consumption channels is supported. Developers can choose to consume
 * lynx logs at either the platform layer or the C++ layer as required.
 * - For platform layer channel: Developers need to overload ILynxLogService::logByPlatform;
 * - For C++ layer channel: Developers are required to introduce the corresponding C/C++ library,
 * overload the function ILynxLogService::getDefaultWriteFunction, and return the address of the C++
 * layer log processing function via this function;
 * */
public interface ILynxLogService extends IServiceProvider {
  public enum LogOutputChannelType { Native, Platform }
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxLogService.class;
  }

  /*
   * Consume lynx logs on the platform layer and it is necessary to implement log capabilities
   * independently.
   * */
  void logByPlatform(int level, String tag, String msg);

  /*
   * Whether to consume lynx logs on the platform layer.
   * If it is true, the logByPlatform function will be called; otherwise, the log function in the
   * C++
   * */
  boolean isLogOutputByPlatform();

  /*
   * For C++ layer channel: Developers are required to introduce the corresponding C/C++ library,
   * Return the address of the C/C++ log function.
   * */
  long getDefaultWriteFunction();

  /*
   * Todo(lipin.1001):maybe remove
   * */
  void switchLogToSystem(boolean enableSystemLog);
  boolean getLogToSystemStatus();
}
