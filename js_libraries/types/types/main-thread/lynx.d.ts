// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxClearTimeout, LynxSetTimeout, CommonLynx } from '../common';
import { CommonPerformance } from '../common/performance';

/**
 * @description Lynx provide `main-thread lynx` public api.
 */
export interface Lynx extends CommonLynx {
  // Currently is internal.
  // setTimeout: LynxSetTimeout;

  // setInterval: LynxSetTimeout;

  // clearTimeout: LynxClearTimeout;

  // clearInterval: LynxClearTimeout;

  // triggerLepusBridge: <Params = Record<string, unknown>>(methodName: string, methodDetail: Params, cb: (...args: unknown[]) => void) => void;
  // triggerLepusBridgeSync: <Ret = unknown, Params = Record<string, unknown>>(methodName: string, methodDetail: Params) => Ret;

  /**
   * Support from Lynx 3.0
   */
  performance: CommonPerformance;
}
