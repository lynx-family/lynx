// Copyright 2025 The Lynx Authors. All rights reserved.

export class DebugBridgeHarmony {
  constructor();

  static initialize: () => void;
}

export class InspectorOwnerHarmony {
  constructor(owner: LynxInspectorOwner, embedderProxy: number[]);

  attachProxy: (embedderProxy: number[]) => void;
  getSessionId: () => number;
  destroy: () => void;
  flushConsoleMessages: () => void;
  getConsoleObject: (objectId: string, needStringify: boolean, callbackId: number) => void;
  subscribeMessage: (type: string, handler: MessageHandler) => void;
  unsubscribeMessage: (type: string) => void;
  updateInputWindowInfo: (windowId: number, displayId: number,
    leftPx: number, topPx: number, widthPx: number, heightPx: number,
    pixelRatio: number) => void;
  invalidateInputWindow: () => void;
}

export class LynxDevToolEnvHarmony {
  static initDevToolEnv: () => void;
  /** @deprecated Use DevToolSettings in @lynx/lynx instead. */
  static setSwitch: (key: string, value: boolean) => void;
  /** @deprecated Use DevToolSettings in @lynx/lynx instead. */
  static getSwitch: (key: string) => boolean;
  static initDevToolSetModule: (moduleManager: number[]) => void;
}
