// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IServiceProvider } from './IServiceProvider';
import { LynxBaseInspectorOwner } from '../../devtool_wrapper/LynxBaseInspectorOwner';
import { GetJSSourceCallback, ILogBox, OnErrorCallback } from '../../devtool_wrapper/ILogBox';
import { NodeController } from '@kit.ArkUI';

export interface ILynxDevToolService extends IServiceProvider {
  isDevToolModuleLoaded(): boolean

  connectDevtools(remoteDebugUrl: string, appInfo: Map<string, string>): boolean

  initDevToolSetModule(moduleManager: number[]): void

  devtoolEnvSetSwitch(key: string, value: boolean): void

  devtoolEnvGetSwitch(key: string): boolean

  devtoolEnvSetAppInfo(optionKeys: Array<string>, optionValues: Array<string>): void

  createInspectorOwner(embedderProxy: number[]): LynxBaseInspectorOwner | null

  createLogBox(onError: OnErrorCallback, getJsSource: GetJSSourceCallback): ILogBox | null

  createLogBoxComponent(logbox: ILogBox, title: string): NodeController | null;
}