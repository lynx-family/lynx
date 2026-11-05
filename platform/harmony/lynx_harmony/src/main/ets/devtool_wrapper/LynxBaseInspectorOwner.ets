// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { CDPEventListener } from './CDPEventListener';
import { LynxInspectorConsoleDelegate } from './LynxInspectorConsoleDelegate';
import { MessageHandler } from './MessageHandler';

export type CDPResultCallback = (result: string) => void;

export interface LynxBaseInspectorOwner {
  setLynxInspectorConsoleDelegate(delegate: LynxInspectorConsoleDelegate): void;

  getConsoleObject(objectId: string, needStringify: boolean, callback: (detail: string) => void): void;

  subscribeMessage(type: string, handler: MessageHandler): void;

  unsubscribeMessage(type: string): void;

  sendMessage(type: string, message: string): void;

  invokeCDPFromSDK(cdpMsg: string, callback: CDPResultCallback): void;

  addCDPEventListener(name: string, listener: CDPEventListener): void;

  removeCDPEventListener(name: string): void;
}

