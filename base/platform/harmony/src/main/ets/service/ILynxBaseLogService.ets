// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IBaseServiceProvider } from './IBaseServiceProvider';

export enum LogOutputChannelType {
  Native,
  Platform,
}

/*
 * The selection of lynx log consumption channels is supported. Developers can choose to consume
 * lynx logs at either the platform layer or the C++ layer as required.
 * - For platform layer channel: Developers need to overload ILynxBaseLogService::logByPlatform;
 * - For C++ layer channel: Developers are required to introduce the corresponding C/C++ library,
 * overload the function ILynxBaseLogService::getDefaultWriteFunction, and return the address of the C++
 * layer log processing function via this function;
 * */
export interface ILynxBaseLogService extends IBaseServiceProvider {
  /*
   * Consume lynx logs on the platform layer and it is necessary to implement log capabilities
   * independently.
   * */
  logByPlatform(level: number, tag: string, message: string): void;

  /*
   * Whether to consume lynx logs on the platform layer.
   * If it is true, the logByPlatform function will be called; otherwise, the log function in the
   * C++
   * */
  isLogOutputByPlatform(): boolean;

  /*
   * For C++ layer channel: Developers are required to introduce the corresponding C/C++ library,
   * Return the address of the C/C++ log function.
   * */
  getDefaultWriteFunction(): number;
}
