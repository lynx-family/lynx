// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxBaseInspectorOwner, LynxView, LynxInspectorConsoleDelegate, MessageHandler } from '@lynx/lynx';
import { DebugRouter } from './DebugRouter';
import { ConsoleDelegateManager } from './ConsoleDelegateManager';
import { LynxBaseInspectorController } from '@lynx/lynx/src/main/ets/devtool_wrapper/LynxBaseInspectorController';

import lynxdevtool from 'liblynxdevtool.so';

export class LynxInspectorOwner implements LynxBaseInspectorOwner, LynxBaseInspectorController {
  private nativeOwner: lynxdevtool.InspectorOwnerHarmony;
  private consoleDelegateManager: ConsoleDelegateManager;
  private sessionId: number = -1;

  constructor(embedderProxy: number[]) {
    this.nativeOwner = new lynxdevtool.InspectorOwnerHarmony(this, embedderProxy);
    this.consoleDelegateManager = new ConsoleDelegateManager();
  }

  public onLoaded(lynxView: LynxView) {
    this.sessionId = this.nativeOwner.getSessionId();
    DebugRouter.setSessionIdOfView(lynxView, this.sessionId);
  }

  public destroy(lynxView: LynxView): void {
    DebugRouter.removeView(lynxView);
    this.nativeOwner.destroy();
  }

  public setLynxInspectorConsoleDelegate(delegate: LynxInspectorConsoleDelegate): void {
    this.consoleDelegateManager.setLynxInspectorConsoleDelegate(delegate, this.nativeOwner);
  }

  public getConsoleObject(objectId: string, needStringify: boolean, callback: (detail: string) => void): void {
    this.consoleDelegateManager.getConsoleObject(objectId, needStringify, callback, this.nativeOwner);
  }

  public onConsoleMessage(msg: string): void {
    this.consoleDelegateManager.onConsoleMessage(msg);
  }

  public onConsoleObject(detail: string, callbackId: number): void {
    this.consoleDelegateManager.onConsoleObject(detail, callbackId);
  }

  public subscribeMessage(type: string, handler: MessageHandler): void {
    this.nativeOwner.subscribeMessage(type, handler);
  }

  public unsubscribeMessage(type: string): void {
    this.nativeOwner.unsubscribeMessage(type);
  }

  public sendMessage(type: string, message: string): void {
    DebugRouter.sendDataAsync(type, this.sessionId, message);
  }
}
