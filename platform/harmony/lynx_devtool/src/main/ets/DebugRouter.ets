// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import type { HarmonyGlobalHandler, HarmonySessionHandler } from 'liblynxdevtool.so';
import { DebugRouterWrapper } from 'liblynxdevtool.so';
import { LynxView } from '@lynx/lynx';

export class DebugRouter {
  private static viewMap: Map<LynxView, number> = new Map<LynxView, number>();

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

  public static handleSchema(url: string): boolean {
    return DebugRouterWrapper.handleSchema(url);
  }
}