// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxInspectorConsoleDelegate } from '@lynx/lynx/src/main/ets/devtool_wrapper/LynxInspectorConsoleDelegate';
import { InspectorOwnerHarmony } from 'liblynxdevtool.so';

export class ConsoleDelegateManager {
  private consoleDelegate?: LynxInspectorConsoleDelegate;
  private consoleObjectCallback?: Map<number, (detail: string) => void>;
  private static nextCallbackId: number = 0;

  public setLynxInspectorConsoleDelegate(delegate: LynxInspectorConsoleDelegate,
    nativeOwner: InspectorOwnerHarmony): void {
    this.consoleDelegate = delegate;
    nativeOwner.flushConsoleMessages();
  }

  public getConsoleObject(objectId: string, needStringify: boolean, callback: (detail: string) => void,
    nativeOwner: InspectorOwnerHarmony): void {
    if (!callback) {
      return;
    }
    if (!this.consoleObjectCallback) {
      this.consoleObjectCallback = new Map();
    }
    let callbackId = ConsoleDelegateManager.nextCallbackId--;
    this.consoleObjectCallback.set(callbackId, callback);
    nativeOwner.getConsoleObject(objectId, needStringify, callbackId);
  }

  public onConsoleMessage(msg: string): void {
    if (this.consoleDelegate) {
      this.consoleDelegate.onConsoleMessage(msg);
    }
  }

  public onConsoleObject(detail: string, callbackId: number): void {
    if (!this.consoleObjectCallback || !this.consoleObjectCallback.has(callbackId)) {
      return;
    }
    let callback = this.consoleObjectCallback.get(callbackId);
    if (!callback) {
      return;
    }
    callback(detail);
    this.consoleObjectCallback.delete(callbackId);
  }
}
