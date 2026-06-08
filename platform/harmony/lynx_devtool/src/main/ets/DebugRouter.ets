// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import util from '@ohos.util';
import type {
  DebugRouterMessageHandler,
  HarmonyGlobalHandler,
  HarmonySessionHandler,
  HarmonyStateListener
} from 'liblynxdevtool.so';
import { DebugRouterWrapper } from 'liblynxdevtool.so';
import { LynxView, LynxEnv } from '@lynx/lynx';

export class DebugRouter {
  private static viewMap: Map<LynxView, number> = new Map<LynxView, number>();
  private static initialized: boolean = false;

  static {
    DebugRouter.init();
  }

  public static init(): void {
    if (DebugRouter.initialized) {
      return;
    }
    let debugRouterId: string = DebugRouter.generateUUID();
    let options = new Map<string, string>();
    options.set('debugRouterId', debugRouterId);
    LynxEnv.setAppInfo(options);
    DebugRouter.initialized = true;
  }

  private static generateUUID(): string {
    let uuid: string = util.generateRandomUUID(true);
    return uuid;
  }

  public static getSessionIdByView(view: LynxView): number {
    let sessionId = DebugRouter.viewMap.get(view);
    return sessionId === undefined ? 0 : sessionId;
  }

  public static getViewBySessionId(sessionId: number): LynxView | null {
    let result: LynxView | null = null;
    DebugRouter.viewMap.forEach((id, view) => {
      if (id == sessionId) {
        result = view;
      }
    });
    return result;
  }

  public static setSessionIdOfView(view: LynxView, sessionId: number): void {
    DebugRouter.viewMap.set(view, sessionId);
  }

  public static removeView(view: LynxView): void {
    DebugRouter.viewMap.delete(view);
  }

  public static addGlobalHandler(handler: HarmonyGlobalHandler): void {
    DebugRouterWrapper.addGlobalHandler(handler);
  }

  public static removeGlobalHandler(handler: HarmonyGlobalHandler): void {
    DebugRouterWrapper.removeGlobalHandler(handler);
  }

  public static sendDataAsync(type: string, session: number, data: string): void {
    DebugRouterWrapper.sendDataAsync(type, session, data);
  }

  public static addSessionHandler(handler: HarmonySessionHandler): void {
    DebugRouterWrapper.addSessionHandler(handler);
  }

  public static addMessageHandler(name: string, handler: DebugRouterMessageHandler): void {
    DebugRouterWrapper.addMessageHandler(name, handler);
  }

  public static removeMessageHandler(name: string): void {
    DebugRouterWrapper.removeMessageHandler(name);
  }

  public static handleSchema(url: string): boolean {
    let result = DebugRouterWrapper.handleSchema(url);
    return result;
  }

  public static getAppInfoByKey(key: string): string {
    return DebugRouterWrapper.getAppInfoByKey(key);
  }

  public static addStateListener(listener: HarmonyStateListener): void {
    DebugRouterWrapper.addStateListener(listener);
  }
}
