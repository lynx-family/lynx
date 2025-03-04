// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { ILogBoxBridge } from '@/jsbridge/interface';
import { ImmBridgeImpl } from './immediate';

let isBridgeReady: Promise<void> | undefined;
export function getBridgeReadyPromise(): Promise<void> {
  isBridgeReady = isBridgeReady ?? new Promise((resolve) => document.addEventListener('LogBoxReady' as any, resolve));
  return isBridgeReady;
}

export const DelayBridgeImpl: ILogBoxBridge = {
  addEventListeners(): void {
    getBridgeReadyPromise().then(() => ImmBridgeImpl.addEventListeners());
  },
  async getErrorData(): Promise<void> {
    await getBridgeReadyPromise();
    ImmBridgeImpl.getErrorData();
  },
  toastMessage(msg): void {
    getBridgeReadyPromise().then(() => ImmBridgeImpl.toastMessage(msg));
  },
  dismissError(): void {
    getBridgeReadyPromise().then(() => ImmBridgeImpl.dismissError());
  },
  async queryResource(url: string): Promise<string> {
    await getBridgeReadyPromise();
    return ImmBridgeImpl.queryResource(url);
  },
  changeView(viewNumber: number): void {
    getBridgeReadyPromise().then(() => ImmBridgeImpl.changeView(viewNumber));
  },
  deleteCurrentView(viewNumber: number): void {
    getBridgeReadyPromise().then(() => ImmBridgeImpl.deleteCurrentView(viewNumber));
  },
};
