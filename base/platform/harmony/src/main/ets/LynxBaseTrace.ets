// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynxbase from 'liblynxbase.so';
import { LynxLog } from './log/LynxLog';
import { LynxBaseServiceCenter, LynxBaseServiceType } from './service/LynxBaseServiceCenter';
import { ILynxBaseTraceService } from './service/ILynxBaseTraceService';

const TAG = 'LynxBaseTrace';

export class LynxBaseTrace {
  private static service?: ILynxBaseTraceService;

  static initLynxBaseTrace(): boolean {
    LynxBaseTrace.service = LynxBaseServiceCenter.getService<ILynxBaseTraceService>(LynxBaseServiceType.Trace);
    if (LynxBaseTrace.service) {
      let address: number | undefined = LynxBaseTrace.service.getDefaultTraceFunction();
      if (address != 0) {
        lynxbase.nativeInitLynxBaseTrace(address);
        LynxLog.i(TAG,
          'LynxBaseTrace init successfully by custom LynxBaseTraceService. function native address is ' + address);
        return true;
      }
    }
    LynxLog.i(TAG, 'failed to init LynxBaseTrace dependency');
    return false;
  }
}
