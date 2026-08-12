// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BusinessError } from '@ohos.base';
import { window } from '@kit.ArkUI';

import {
  LLog,
  LynxBaseInspectorOwner,
  LynxEnv,
  LynxInspectorConsoleDelegate,
  LynxView,
  MessageHandler
} from '@lynx/lynx';
import { DebugRouter } from './DebugRouter';
import { ConsoleDelegateManager } from './ConsoleDelegateManager';
import { LynxBaseInspectorController } from '@lynx/lynx/src/main/ets/devtool_wrapper/LynxBaseInspectorController';

import lynxdevtool from 'liblynxdevtool.so';

const TAG = 'LynxInspectorOwner';

export class LynxInspectorOwner implements LynxBaseInspectorOwner, LynxBaseInspectorController {
  private nativeOwner: lynxdevtool.InspectorOwnerHarmony;
  private consoleDelegateManager: ConsoleDelegateManager;
  private sessionId: number = -1;
  private inputWindow?: window.Window;
  private windowRectChangeCallback?: (options: window.RectChangeOptions) => void;
  private displayIdChangeCallback?: (displayId: number) => void;
  private windowRequestId: number = 0;
  private destroyed: boolean = false;

  constructor(embedderProxy: number[]) {
    this.nativeOwner = new lynxdevtool.InspectorOwnerHarmony(this, embedderProxy);
    this.consoleDelegateManager = new ConsoleDelegateManager();
  }

  public attachProxy(embedderProxy: number[]): void {
    this.nativeOwner.attachProxy(embedderProxy);
  }

  public onLoaded(lynxView: LynxView) {
    this.sessionId = this.nativeOwner.getSessionId();
    DebugRouter.setSessionIdOfView(lynxView, this.sessionId);
    const requestId = ++this.windowRequestId;
    this.detachInputWindow();
    this.nativeOwner.invalidateInputWindow();
    lynxView.getCurrentWindow().then((win: window.Window) => {
      if (this.destroyed || requestId !== this.windowRequestId) {
        return;
      }
      this.attachInputWindow(win);
    }).catch((error: BusinessError) => {
      if (this.destroyed || requestId !== this.windowRequestId) {
        return;
      }
      this.nativeOwner.invalidateInputWindow();
      LLog.e(TAG, `Failed to resolve the host window. Code: ${error.code}, message: ${error.message}`);
    });
  }

  public destroy(lynxView: LynxView): void {
    this.destroyed = true;
    ++this.windowRequestId;
    this.detachInputWindow();
    this.nativeOwner.invalidateInputWindow();
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

  private attachInputWindow(win: window.Window): void {
    this.inputWindow = win;
    this.windowRectChangeCallback = () => this.syncInputWindow(win);
    this.displayIdChangeCallback = () => this.syncInputWindow(win);
    this.syncInputWindow(win);
    try {
      win.on('windowRectChange', this.windowRectChangeCallback);
    } catch (error) {
      LLog.w(TAG, `Failed to observe window bounds: ${error}`);
    }
    try {
      win.on('displayIdChange', this.displayIdChangeCallback);
    } catch (error) {
      LLog.w(TAG, `Failed to observe the window display: ${error}`);
    }
  }

  private detachInputWindow(): void {
    const inputWindow = this.inputWindow;
    if (inputWindow && this.windowRectChangeCallback) {
      try {
        inputWindow.off('windowRectChange', this.windowRectChangeCallback);
      } catch (error) {
        LLog.w(TAG, `Failed to stop observing window bounds: ${error}`);
      }
    }
    if (inputWindow && this.displayIdChangeCallback) {
      try {
        inputWindow.off('displayIdChange', this.displayIdChangeCallback);
      } catch (error) {
        LLog.w(TAG, `Failed to stop observing the window display: ${error}`);
      }
    }
    this.inputWindow = undefined;
    this.windowRectChangeCallback = undefined;
    this.displayIdChangeCallback = undefined;
  }

  private syncInputWindow(win: window.Window): void {
    if (this.destroyed || this.inputWindow !== win) {
      return;
    }
    try {
      const properties = win.getWindowProperties();
      const rect = properties.windowRect;
      this.nativeOwner.updateInputWindowInfo(
        properties.id,
        properties.displayId ?? 0,
        Math.round(rect.left),
        Math.round(rect.top),
        Math.round(rect.width),
        Math.round(rect.height),
        LynxEnv.getDensity());
    } catch (error) {
      this.nativeOwner.invalidateInputWindow();
      LLog.w(TAG, `Failed to synchronize the host window: ${error}`);
    }
  }
}
