// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxModule } from '@lynx/lynx';

export default class LynxRecorderReplayDataModule extends LynxModule {
  static jsbInvokers: Map<string, Object>[] = [];
  static jsbCallbacks: Map<string, Object> = new Map();
  static config: Map<string, Object> = new Map();

  public getData(callback: (str: string) => void) {
    let json: Record<string, string> = {};
    json['RecordData'] = this.getRecordData();
    json['JsbIgnoredInfo'] = this.getJsbIgnoredInfo();
    json['JsbSettings'] = this.getJsbSettings();
    callback(JSON.stringify(json));
  }

  private getRecordData() {
    let json: Record<string, Object[]> = {};
    for (let index = 0; index < LynxRecorderReplayDataModule.jsbInvokers.length; index++) {
      let functionInvoker: Map<string, Object> = LynxRecorderReplayDataModule.jsbInvokers[index];
      let moduleName = functionInvoker['Module Name'] as string;
      if (!json[moduleName]) {
        json[moduleName] = [];
      }
      let methodLookUp: Record<string, Object> = {};
      let methodName = functionInvoker['Method Name'] as string;
      let requestTime = (functionInvoker['Record Time'] as number) * 1000;
      if (functionInvoker['RecordMillisecond']) {
        requestTime = functionInvoker['RecordMillisecond'] as number;
      }

      let params: Map<string, Object> = functionInvoker['Params'] as Map<string, Object>;
      let callbackIDS: string[] = params['callback'] as string[];
      let callbackReturnValues: Object[] = [];
      if (callbackIDS !== undefined) {
        for (let i = 0; i < callbackIDS.length; i++) {
          let callbackInfo = LynxRecorderReplayDataModule.jsbCallbacks![callbackIDS[i]] as Map<string, Object>;
          if (callbackInfo) {
            let responseTime = (callbackInfo['Record Time'] as number) * 1000;
            if (callbackInfo['RecordMillisecond']) {
              responseTime = callbackInfo['RecordMillisecond'] as number;
            }
            let callbackKernel: Map<string, Object> = new Map();
            callbackKernel['Value'] = callbackInfo['Params'] as Object;
            callbackKernel['Delay'] = responseTime - requestTime;
            callbackReturnValues.push(callbackKernel);
          }
        }
      }


      methodLookUp['Method Name'] = methodName;
      methodLookUp['Params'] = params;
      methodLookUp['Callback'] = callbackReturnValues;
      methodLookUp['Label'] = callbackIDS === undefined ? -1 : callbackIDS[0];
      json[moduleName].push(methodLookUp);
    }
    let result = JSON.stringify(json);
    return result;
  }

  private getJsbIgnoredInfo() {
    if (!LynxRecorderReplayDataModule.config || !LynxRecorderReplayDataModule.config['jsbIgnoredInfo'])
      return '[]';
    let ignoreInfo: string[] = LynxRecorderReplayDataModule.config['jsbIgnoredInfo'];
    return JSON.stringify(ignoreInfo);
  }

  private getJsbSettings() {
    return '{}';
  }
}
