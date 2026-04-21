// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { ILynxDevToolService, LLog, LynxEnv, LynxModuleManager } from '@lynx/lynx';
import { GetJSSourceCallback, ILogBox, OnErrorCallback } from '@lynx/lynx/src/main/ets/devtool_wrapper/ILogBox';
import { LynxBaseInspectorController } from '@lynx/lynx/src/main/ets/devtool_wrapper/LynxBaseInspectorController';
import { NodeController } from '@kit.ArkUI';

export class LynxDevToolService implements ILynxDevToolService {
  private static TAG = 'LynxDevToolService';
  private static devtoolModule?: ESObject;
  static instance = new LynxDevToolService();
  static {
    const devtool = "@lynx/lynx_devtool";
    import(devtool)
      .then((ns: ESObject) => {
        LynxDevToolService.devtoolModule = ns;
        LynxEnv.tryInitLynxDebug();
      }).catch((e: Error) => {
      LLog.e(LynxDevToolService.TAG, 'load devtool error: ' + e);
    });
  }

  private constructor() {
  }

  public isDevToolModuleLoaded(): boolean {
    return LynxDevToolService.devtoolModule !== undefined;
  }

  public connectDevtools(remoteDebugUrl: string, appInfo: Map<string, string>): boolean {
    let lynxDebugBridgeInst: ESObject = LynxDevToolService.devtoolModule?.LynxDebugBridge?.getInstance();
    if (lynxDebugBridgeInst) {
      return lynxDebugBridgeInst.connectDevtools(remoteDebugUrl, appInfo);
    } else {
      LLog.i(LynxDevToolService.TAG, "connectDevtools lynxDebugBridgeInst is null");
      return false;
    }
  }

  public initDevToolSetModule(moduleManager: number[]): void {
    let lynxDevToolEnv: ESObject = LynxDevToolService.devtoolModule?.LynxDevToolEnv;
    if (lynxDevToolEnv) {
      lynxDevToolEnv.initDevToolSetModule(moduleManager);
    } else {
      LLog.i(LynxDevToolService.TAG, "initDevToolSetModule lynxDevToolEnv is null");
    }
  }

  /** @deprecated Use DevToolSettings instead. */
  public devtoolEnvSetSwitch(key: string, value: boolean): void {
    let lynxDevToolEnv: ESObject = LynxDevToolService.devtoolModule?.LynxDevToolEnv;
    if (lynxDevToolEnv) {
      lynxDevToolEnv.setSwitch(key, value);
    } else {
      LLog.i(LynxDevToolService.TAG, "devtoolEnvSetSwitch lynxDevToolEnv is null");
    }
  }

  /** @deprecated Use DevToolSettings instead. */
  public devtoolEnvGetSwitch(key: string): boolean {
    let lynxDevToolEnv: ESObject = LynxDevToolService.devtoolModule?.LynxDevToolEnv;
    if (lynxDevToolEnv) {
      return lynxDevToolEnv.getSwitch(key);
    } else {
      LLog.i(LynxDevToolService.TAG, "devtoolEnvGetSwitch lynxDevToolEnv is null");
      return false;
    }
  }

  public devtoolEnvSetAppInfo(optionKeys: Array<string>, optionValues: Array<string>): void {
    let lynxDevToolEnv: ESObject = LynxDevToolService.devtoolModule?.LynxDevToolEnv;
    if (lynxDevToolEnv) {
      lynxDevToolEnv.setAppInfo(optionKeys, optionValues);
    } else {
      LLog.i(LynxDevToolService.TAG, "devtoolEnvSetAppInfo lynxDevToolEnv is null");
    }
  }

  public createInspectorOwner(embedderProxy: number[]): LynxBaseInspectorController | null {
    let lynxInspectorOwner: ESObject = LynxDevToolService.devtoolModule?.LynxInspectorOwner;
    if (lynxInspectorOwner) {
      return new lynxInspectorOwner(embedderProxy);
    } else {
      LLog.i(LynxDevToolService.TAG, "createInspectorOwner lynxInspectorOwner is null");
      return null;
    }
  }

  public createLogBox(onError: OnErrorCallback, getJsSource: GetJSSourceCallback): ILogBox | null {
    let LogBox: ESObject = LynxDevToolService.devtoolModule?.LogBox;
    if (LogBox) {
      return new LogBox(onError, getJsSource);
    } else {
      LLog.i(LynxDevToolService.TAG, "createLogBox logbox is null");
      return null;
    }
  }

  public createLogBoxComponent(logBox: ILogBox, title: string): NodeController | null {
    let LogBoxController: ESObject = LynxDevToolService.devtoolModule?.LogBoxNodeController;
    if (LogBoxController) {
      return new LogBoxController(logBox, title);
    } else {
      LLog.i(LynxDevToolService.TAG, "createLogBoxComponent logbox is null");
      return null;
    }
  }

  public registerWebSocketModule(moduleManager: LynxModuleManager): void {
    let LynxWebSocketModule: ESObject = LynxDevToolService.devtoolModule?.LynxWebSocketModule;
    if (LynxWebSocketModule) {
      moduleManager.registerModule('LynxWebSocketModule', { moduleClass: LynxWebSocketModule });
    }
  }

  public addOpenCardCallback(callback: (url: string) => void): void {
    let lynxDebugBridgeInst: ESObject = LynxDevToolService.devtoolModule?.LynxDebugBridge?.getInstance();
    if (lynxDebugBridgeInst) {
      lynxDebugBridgeInst.addOpenCardCallback(callback);
    } else {
      LLog.i(LynxDevToolService.TAG, "addOpenCardCallback failed, lynxDebugBridgeInst is null");
    }
  }
}
