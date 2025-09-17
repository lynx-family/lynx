// Copyright 2025 The Lynx Authors. All rights reserved.

export class DebugBridgeHarmony {
  constructor();

  static connectDevtools: (remoteDebugUrl: string, optionKeys: Array<string>,
    optionValues: Array<string>) => boolean;
}

export class InspectorOwnerHarmony {
  constructor(owner: LynxInspectorOwner, embedderProxy: number[]);

  getSessionId: () => number;
  destroy: () => void;
  flushConsoleMessages: () => void;
  getConsoleObject: (objectId: string, needStringify: boolean, callbackId: number) => void;
}

export class LynxDevToolEnvHarmony {
  static setSwitch: (key: string, value: boolean) => void;
  static getSwitch: (key: string) => boolean;
  static setAppInfo: (optionKeys: Array<string>, optionValues: Array<string>) => void;
  static initDevToolSetModule: (moduleManager: number[]) => void;
}

export interface HarmonyGlobalHandler {
  onOpenCard: (url: string) => void;
  onMessage: (message: string, type: string) => void;
}

export interface HarmonySessionHandler {
  OnSessionCreate: (session_id: number, url: string) => void;
  OnSessionDestroy: (session_id: number) => void;
  OnMessage: (message: string, type: string, session_id: number) => void;
}

export declare class DebugRouterWrapper {
  static addGlobalHandler: (handler: HarmonyGlobalHandler) => void;
  static removeGlobalHandler: (handler: HarmonyGlobalHandler) => void;
  static sendDataAsync: (type: string, session: number, data: string) => void;
  static addSessionHandler: (handler: HarmonySessionHandler) => void;
  static handleSchema: (url: string) => boolean;
}
