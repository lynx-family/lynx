// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IBaseServiceProvider } from './IBaseServiceProvider';

/*
 * Developers are required to introduce the corresponding C/C++ library, overload the function
 * ILynxBaseTraceService::getDefaultTraceFunction, and return the address of the C++ layer log
 * processing function via this function.
 * */
export interface ILynxBaseTraceService extends IBaseServiceProvider {
  /*
   * For C++ layer channel: Developers are required to introduce the corresponding C/C++ library,
   * Return the address of the C/C++ trace function.
   * */
  getDefaultTraceFunction(): number;
}
