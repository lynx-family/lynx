// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IServiceProvider } from './IServiceProvider';
import { GetJSSourceCallback, ILogBox, OnErrorCallback } from '../../devtool_wrapper/ILogBox';
import { NodeController } from '@kit.ArkUI';
import { LynxModuleManager } from '../../jsbridge/LynxModuleManager';
import { LynxBaseInspectorController } from '../../devtool_wrapper/LynxBaseInspectorController';

export interface ILynxDevToolService extends IServiceProvider {
  isDevToolModuleLoaded(): boolean

  connectDevtools(remoteDebugUrl: string, appInfo: Map<string, string>): boolean

  initDevToolSetModule(moduleManager: number[]): void

  /** @deprecated Use DevToolSettings instead. */
  devtoolEnvSetSwitch(key: string, value: boolean): void

  /** @deprecated Use DevToolSettings instead. */
  devtoolEnvGetSwitch(key: string): boolean

  devtoolEnvSetAppInfo(optionKeys: Array<string>, optionValues: Array<string>): void

  createInspectorOwner(embedderProxy: number[]): LynxBaseInspectorController | null

  createLogBox(onError: OnErrorCallback, getJsSource: GetJSSourceCallback): ILogBox | null

  createLogBoxComponent(logbox: ILogBox, title: string): NodeController | null;

  registerWebSocketModule(moduleManager: LynxModuleManager): void;

  addOpenCardCallback(callback: (url: string) => void): void;
}
