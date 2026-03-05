// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynxdevtool from 'liblynxdevtool.so';
import { DebugRouter } from './DebugRouter';

export class LynxDebugBridge {
  private static instance?: LynxDebugBridge;

  private constructor() {
  }

  public static getInstance(): LynxDebugBridge {
    if (LynxDebugBridge.instance) {
      return LynxDebugBridge.instance;
    }
    LynxDebugBridge.instance = new LynxDebugBridge();
    return LynxDebugBridge.instance;
  }


  public connectDevtools(remoteDebugUrl: string, appInfo: Map<string, string>): boolean {
    return lynxdevtool.DebugBridgeHarmony.connectDevtools(remoteDebugUrl, Array.from(appInfo.keys()),
      Array.from(appInfo.values()));
  }

  public addOpenCardCallback(callback: (url: string) => void): void {
    DebugRouter.addGlobalHandler({
      onOpenCard: callback,
      onMessage: () => {
      }
    });
  }
}
