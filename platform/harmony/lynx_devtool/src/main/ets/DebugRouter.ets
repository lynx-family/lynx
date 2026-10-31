// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type {
  DebugRouterGlobalHandlerHarmony,
  DebugRouterMessageHandlerHarmony,
  DebugRouterSessionHandlerHarmony,
  DebugRouterStateListenerHarmony
} from '@lynx/debug_router';
import { DebugRouter as HarmonyDebugRouter } from '@lynx/debug_router';
import { LynxView } from '@lynx/lynx';

export type HarmonyGlobalHandler = DebugRouterGlobalHandlerHarmony;
export type HarmonySessionHandler = DebugRouterSessionHandlerHarmony;
export type HarmonyStateListener = DebugRouterStateListenerHarmony;

export interface DebugRouterMessageHandler {
  handle: (params: string) => string;
}

export class DebugRouter {
  private static messageHandlers: Map<string, DebugRouterMessageHandlerHarmony> =
    new Map<string, DebugRouterMessageHandlerHarmony>();
  private static initialized: boolean = false;

  static {
    DebugRouter.init();
  }

  public static init(): void {
    if (DebugRouter.initialized) {
      return;
    }
    HarmonyDebugRouter.createInstance();
    DebugRouter.initialized = true;
  }

  public static getSessionIdByView(view: LynxView): number {
    return HarmonyDebugRouter.getSessionIdByView(view);
  }

  public static getViewBySessionId(sessionId: number): LynxView | null {
    return HarmonyDebugRouter.getViewBySessionId(sessionId) as LynxView | null;
  }

  public static setSessionIdOfView(view: LynxView, sessionId: number): void {
    HarmonyDebugRouter.setSessionIdOfView(view, sessionId);
  }

  public static removeView(view: LynxView): void {
    HarmonyDebugRouter.removeView(view);
  }

  public static addGlobalHandler(handler: HarmonyGlobalHandler): void {
    HarmonyDebugRouter.addGlobalHandler(handler);
  }

  public static removeGlobalHandler(handler: HarmonyGlobalHandler): void {
    HarmonyDebugRouter.removeGlobalHandler(handler);
  }

  public static sendDataAsync(type: string, session: number, data: string): void {
    HarmonyDebugRouter.sendDataAsync(type, session, data);
  }

  public static addSessionHandler(handler: HarmonySessionHandler): void {
    HarmonyDebugRouter.addSessionHandler(handler);
  }

  public static addMessageHandler(name: string, handler: DebugRouterMessageHandler): void {
    const oldHandler: DebugRouterMessageHandlerHarmony | undefined = DebugRouter.messageHandlers.get(name);
    if (oldHandler !== undefined) {
      HarmonyDebugRouter.removeMessageHandler(oldHandler);
    }
    const harmonyHandler: DebugRouterMessageHandlerHarmony = {
      handle: (params: string): string => handler.handle(params),
      getName: (): string => name
    };
    DebugRouter.messageHandlers.set(name, harmonyHandler);
    HarmonyDebugRouter.addMessageHandler(harmonyHandler);
  }

  public static removeMessageHandler(name: string): void {
    const handler: DebugRouterMessageHandlerHarmony | undefined = DebugRouter.messageHandlers.get(name);
    if (handler === undefined) {
      return;
    }
    HarmonyDebugRouter.removeMessageHandler(handler);
    DebugRouter.messageHandlers.delete(name);
  }

  public static handleSchema(url: string): boolean {
    return HarmonyDebugRouter.handleSchema(url);
  }

  public static isValidSchema(url: string): boolean {
    return HarmonyDebugRouter.isValidSchema(url);
  }

  public static isConnected(): boolean {
    return HarmonyDebugRouter.isConnected();
  }

  public static getAppInfoByKey(key: string): string {
    return HarmonyDebugRouter.getAppInfoByKey(key);
  }

  public static addStateListener(listener: HarmonyStateListener): void {
    HarmonyDebugRouter.addStateListener(listener);
  }

  public static setAppInfo(appInfo: Map<string, string>): void {
    HarmonyDebugRouter.setAppInfo(appInfo);
  }

  public static enableAllSessions(): void {
    HarmonyDebugRouter.enableAllSessions();
  }
}
