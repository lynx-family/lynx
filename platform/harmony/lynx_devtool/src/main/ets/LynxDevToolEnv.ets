// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynxdevtool from 'liblynxdevtool.so';
import { DebugRouter } from './DebugRouter';

export class LynxDevToolEnv {
  public static initDevToolEnv(): void {
    lynxdevtool.DebugBridgeHarmony.initialize();
    DebugRouter.init();
    DebugRouter.enableAllSessions();
    lynxdevtool.LynxDevToolEnvHarmony.initDevToolEnv();
  }

  /** @deprecated Use DevToolSettings in @lynx/lynx instead. */
  public static setSwitch(key: string, value: boolean) {
    lynxdevtool.LynxDevToolEnvHarmony.setSwitch(key, value);
  }

  /** @deprecated Use DevToolSettings in @lynx/lynx instead. */
  public static getSwitch(key: string): boolean {
    return lynxdevtool.LynxDevToolEnvHarmony.getSwitch(key);
  }

  public static setAppInfo(optionKeys: Array<string>, optionValues: Array<string>): void {
    if (optionKeys.length !== optionValues.length) {
      return;
    }
    const appInfo = new Map<string, string>();
    optionKeys.forEach((key: string, index: number) => {
      appInfo.set(key, optionValues[index]);
    });
    DebugRouter.setAppInfo(appInfo);
  }

  public static initDevToolSetModule(moduleManager: number[]): void {
    lynxdevtool.LynxDevToolEnvHarmony.initDevToolSetModule(moduleManager);
  }
}
