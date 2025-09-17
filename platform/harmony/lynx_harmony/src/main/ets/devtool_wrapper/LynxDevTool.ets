// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxBaseInspectorOwner } from './LynxBaseInspectorOwner';
import { LynxEnv } from '../tasm/LynxEnv';
import { LynxServiceCenter, LynxServiceType } from '../tasm/service/LynxServiceCenter';
import { ILynxDevToolService } from '../tasm/service/ILynxDevToolService';
import { LynxView } from '../tasm/LynxView';
import { GetJSSourceCallback, ILogBox, OnErrorCallback } from './ILogBox';
import { NodeController } from '@kit.ArkUI';

export class LynxDevTool {
  private owner: LynxBaseInspectorOwner | null = null;

  constructor(embedderProxy: number[]) {
    this.initDevTool(embedderProxy);
  }

  private initDevTool(embedderProxy: number[]) {
    if (LynxEnv.isLynxDebugEnabled()) {
      if (LynxEnv.isDevtoolEnabled()) {
        const devToolService = LynxServiceCenter.getService<ILynxDevToolService>(LynxServiceType.DevTool);
        if (devToolService?.isDevToolModuleLoaded()) {
          this.owner = devToolService.createInspectorOwner(embedderProxy) ?? null;
        }
      }
    }
  }
  
  public getBaseInspectorOwner() : LynxBaseInspectorOwner | null {
    return this.owner;
  }

  public onLoaded(lynxView: LynxView) {
    if (this.owner) {
      this.owner.onLoaded(lynxView);
    }
  }

  public destroy(lynxView: LynxView) {
    this.owner?.destroy(lynxView);
    this.owner = null;
  }

  public initDevToolSetModule(moduleManager: number[]) {
    const devToolService = LynxServiceCenter.getService<ILynxDevToolService>(LynxServiceType.DevTool);
    if (devToolService?.isDevToolModuleLoaded()) {
      devToolService.initDevToolSetModule(moduleManager);
    }
  }

  public static createLogBox(onError: OnErrorCallback, getJSSourceCallback: GetJSSourceCallback): ILogBox | null {
    const devToolService = LynxServiceCenter.getService<ILynxDevToolService>(LynxServiceType.DevTool);
    if (devToolService?.isDevToolModuleLoaded()) {
      return devToolService.createLogBox(onError, getJSSourceCallback) ?? null;
    }
    return null;
  }

  public static createLogBoxComponent(logbox: ILogBox, title: string): NodeController | null {
    const devToolService = LynxServiceCenter.getService<ILynxDevToolService>(LynxServiceType.DevTool);
    if (devToolService?.isDevToolModuleLoaded()) {
      return devToolService.createLogBoxComponent(logbox, title) ?? null;
    }
    return null;
  }
}
