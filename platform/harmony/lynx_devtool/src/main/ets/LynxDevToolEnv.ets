// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynxdevtool from 'liblynxdevtool.so';

export class LynxDevToolEnv {
  /** @deprecated Use DevToolSettings in @lynx/lynx instead. */
  public static setSwitch(key: string, value: boolean) {
    lynxdevtool.LynxDevToolEnvHarmony.setSwitch(key, value);
  }

  /** @deprecated Use DevToolSettings in @lynx/lynx instead. */
  public static getSwitch(key: string): boolean {
    return lynxdevtool.LynxDevToolEnvHarmony.getSwitch(key);
  }

  public static setAppInfo(optionKeys: Array<string>, optionValues: Array<string>): void {
    lynxdevtool.LynxDevToolEnvHarmony.setAppInfo(optionKeys, optionValues);
  }

  public static initDevToolSetModule(moduleManager: number[]): void {
    lynxdevtool.LynxDevToolEnvHarmony.initDevToolSetModule(moduleManager);
  }
}
