// Copyright 2025 The Lynx Authors. All rights reserved.

export class DebugBridgeHarmony {
  constructor();

  static connectDevtools: (remoteDebugUrl: string, optionKeys: Array<string>,
    optionValues: Array<string>) => boolean;
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
}

export class LynxDevToolEnvHarmony {
  static initDevToolEnv: () => void;
  /** @deprecated Use DevToolSettings in @lynx/lynx instead. */
  static setSwitch: (key: string, value: boolean) => void;
  /** @deprecated Use DevToolSettings in @lynx/lynx instead. */
  static getSwitch: (key: string) => boolean;
  static setAppInfo: (optionKeys: Array<string>, optionValues: Array<string>) => void;
  static initDevToolSetModule: (moduleManager: number[]) => void;
}

export interface HarmonyGlobalHandler {
  onOpenCard: (url: string) => void;
  onMessage: (message: string, type: string) => void;
}

export interface HarmonySessionHandler {
  onSessionCreate: (session_id: number, url: string) => void;
  onSessionDestroy: (session_id: number) => void;
  onMessage: (message: string, type: string, session_id: number) => void;
}

export interface HarmonyStateListener {
  onOpen: (connectionType: string) => void;
  onClose: (code: number, reason: string) => void;
  onMessage: (message: string) => void;
  onError: (error: string) => void;
}

export interface DebugRouterMessageHandler {
  handle: (params: string) => string;
}

export declare class DebugRouterWrapper {
  static addGlobalHandler: (handler: HarmonyGlobalHandler) => void;
  static removeGlobalHandler: (handler: HarmonyGlobalHandler) => void;
  static addMessageHandler: (name: string, handler: DebugRouterMessageHandler) => void;
  static removeMessageHandler: (name: string) => void;
  static sendDataAsync: (type: string, session: number, data: string) => void;
  static addSessionHandler: (handler: HarmonySessionHandler) => void;
  static handleSchema: (url: string) => boolean;
  static getAppInfoByKey: (key:string) => string;
  static addStateListener: (listener: HarmonyStateListener) => void;
}
