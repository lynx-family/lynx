// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxLog } from './log/LynxLog';
import { LynxBaseTrace } from './LynxBaseTrace';

const TAG = 'LynxBaseEnv';

class LynxBaseEnvironment {
  private initialized = false;

  public initialize(printLogsToAllChannels: boolean) {
    if (this.initialized) {
      return;
    }
    LynxLog.initLynxLog(printLogsToAllChannels);
    LynxBaseTrace.initLynxBaseTrace();
    this.initialized = true;
  }
}

export const LynxBaseEnv = new LynxBaseEnvironment();
