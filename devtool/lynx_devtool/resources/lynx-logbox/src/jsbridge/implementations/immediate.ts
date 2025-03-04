// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { ILogBoxBridge } from '@/jsbridge/interface';
import { loadFile } from '@/utils/fileLoader';
import { clearErrors, clearErrorCache, pushErrorCache } from '@/models/errorReducer';
import { updateViewsInfo } from '@/models/viewsInfo';
import { getAppStore } from '@/models/store';
import { add } from '@/models/errorEffects';

const globalJSBridgeName: string = 'logbox';
const maxImmediateHandleErrorNumber: number = 5;
let receiveErrorCount: number = 0;

function callBridge<T, K>(funcName: string, callback: (payload: T) => void, opt: K): void {
  const bridge = (window as any)[globalJSBridgeName];
  bridge.call(funcName, callback, opt);
}

function addEventListener(event: string, handler: (...args: unknown[]) => unknown): void {
  const bridge = (window as any)[globalJSBridgeName];
  bridge.on(event, handler);
}

function onNewLog(log: any) {
  console.log('onNewLog: ', log);
  if (receiveErrorCount < maxImmediateHandleErrorNumber) {
    add(log);
    receiveErrorCount += 1;
  } else {
    getAppStore().dispatch(pushErrorCache(log));
  }
}

export const ImmBridgeImpl: ILogBoxBridge = {
  addEventListeners(): void {
    addEventListener('receiveNewError', (error: any) => {
      onNewLog(error);
    });
    addEventListener('receiveNewWarning', (warning: any) => {
      onNewLog(warning);
    });
    addEventListener('receiveViewInfo', (groupsInfo) => {
      console.log('receiveViewInfo', groupsInfo);
      getAppStore().dispatch(updateViewsInfo(groupsInfo));
    });
    addEventListener('reset', () => {
      console.log('reset');
      getAppStore().dispatch(clearErrors());
      getAppStore().dispatch(clearErrorCache());
      receiveErrorCount = 0;
    });
    addEventListener('loadFile', (dataPayload) => {
      console.log('loadFile');
      loadFile(dataPayload);
    });
  },
  getErrorData(): void {
    callBridge('getExceptionStack', () => undefined, {});
  },
  toastMessage(msg): void {
    callBridge('toast', () => undefined, { message: msg });
  },
  dismissError(): void {
    receiveErrorCount = 0;
    callBridge('dismiss', () => undefined, {});
  },
  queryResource(url: string): Promise<string> {
    return new Promise<string>((resolve, reject) =>
      callBridge(
        'queryResource',
        (res: unknown) => {
          resolve(`${res}`);
        },
        { name: url },
      ),
    );
  },
  changeView(viewNumber: number): void {
    receiveErrorCount = 0;
    callBridge('changeView', () => undefined, { viewNumber });
  },
  deleteCurrentView(viewNumber: number): void {
    receiveErrorCount = 0;
    callBridge('deleteLynxview', () => {}, { viewNumber });
  },
};
