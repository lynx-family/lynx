// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
import { LLog } from '../../../../Index';

import { LynxServiceCenter, LynxServiceType } from '../tasm/service/LynxServiceCenter';
import { ILynxDevToolService } from '../tasm/service/ILynxDevToolService';

export class LynxDebugger {
  private static TAG: string = '[LynxDebugger]';

  public static connectDevtools(remoteDebugUrl: string, appInfo: Map<string, string>): boolean {
    const devToolService = LynxServiceCenter.getService<ILynxDevToolService>(LynxServiceType.DevTool);
    if (devToolService?.isDevToolModuleLoaded()) {
      return devToolService.connectDevtools(remoteDebugUrl, appInfo) ?? false;
    } else {
      LLog.e(LynxDebugger.TAG, 'fail to get DevTool module');
    }
    return false;
  }

  /** @deprecated Use DevToolSettings instead. */
  public static setSwitch(key: string, value: boolean): void {
    const devToolService = LynxServiceCenter.getService<ILynxDevToolService>(LynxServiceType.DevTool);
    if (devToolService?.isDevToolModuleLoaded()) {
      devToolService.devtoolEnvSetSwitch(key, value);
    } else {
      LLog.e(LynxDebugger.TAG, 'fail to get DevTool module');
    }
  }

  /** @deprecated Use DevToolSettings instead. */
  public static getSwitch(key: string): boolean {
    const devToolService = LynxServiceCenter.getService<ILynxDevToolService>(LynxServiceType.DevTool);
    if (devToolService?.isDevToolModuleLoaded()) {
      return devToolService.devtoolEnvGetSwitch(key) ?? false;
    }
    LLog.e(LynxDebugger.TAG, 'fail to get DevTool module');
    return false;
  }

  public static setAppInfo(optionKeys: Array<string>, optionValues: Array<string>): void {
    const devToolService = LynxServiceCenter.getService<ILynxDevToolService>(LynxServiceType.DevTool);
    if (devToolService?.isDevToolModuleLoaded()) {
      devToolService.devtoolEnvSetAppInfo(optionKeys, optionValues);
    } else {
      LLog.e(LynxDebugger.TAG, 'fail to get DevTool module');
    }
  }

  public static addOpenCardCallback(callback: (url: string) => void): void {
    const devToolService = LynxServiceCenter.getService<ILynxDevToolService>(LynxServiceType.DevTool);
    if (devToolService?.isDevToolModuleLoaded()) {
      devToolService.addOpenCardCallback(callback);
    } else {
      LLog.e(LynxDebugger.TAG, 'fail to get DevTool module');
    }
  }
}
