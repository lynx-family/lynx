// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxModule } from '@lynx/lynx';

export class JSBTestModule extends LynxModule {
  public testAsyncCallBack(value: string[], callback: (value: string[]) => void): void {
    callback(value);
  }

  public testAsyncMultiCallBack(value: string[], callback: (value: string[]) => void,
    callback2: (value: string[]) => void): void {
    callback(value);
    callback2(value);
  }

  public call(funcName: string, params: object, callback: (params: object) => void): void {
    if (funcName === 'asyncTiming') {
      setTimeout(() => {
        callback(params);
      }, 3000);
    } else if (funcName === 'timing') {
      callback(params);
    }
  }
}
