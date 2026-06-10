// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxModule, ModuleClassWrapper, SendableLynxModule, SendableModuleClassWrapper } from './LynxModule';
import { LynxContext } from '../tasm/LynxContext';
import { LLog } from '../base/LLog';
import { constructModule, constructSendableModule } from './Util';
import { collections } from '@kit.ArkTS';

export class LynxModuleManager {
  private modulesByName: Map<string, LynxModule> = new Map();
  private modulesMap: Map<string, ModuleClassWrapper> = new Map();
  private context?: LynxContext;
  private static TAG: string = 'LynxModuleManager';

  constructor(context: LynxContext | undefined) {
    this.context = context;
  }
  
  setContext(context: LynxContext) {
    this.context = context;
  }

  registerModule(name: string, wrapper: ModuleClassWrapper) {
    if (this.modulesMap.has(name)) {
      LLog.e(
        LynxModuleManager.TAG,
        `Duplicated LynxModule For Name: ${name}, will be override`
      );
    }
    this.modulesMap.set(name, wrapper);
    LLog.d(LynxModuleManager.TAG, `registered module with name: ${name}`);
  }

  getModulesMap() {
    return this.modulesMap;
  }

  getModule(name: string): LynxModule | undefined {
    if (name == undefined || name.length == 0) {
      LLog.e(LynxModuleManager.TAG, 'getModule failed, name is null');
      return;
    }
    if (this.modulesByName.get(name) != undefined) {
      return this.modulesByName.get(name);
    }

    const wrapper = this.modulesMap.get(name);
    if (!wrapper) {
      return;
    }
    let module = constructModule(wrapper.moduleClass, this.context, wrapper.param) as LynxModule;
    this.modulesByName.set(name, module);
    return module;
  }
}

@Sendable
export class SendableLynxModuleManager {
  private modulesByName: collections.Map<string, SendableLynxModule> = new collections.Map();
  private modulesMap: collections.Map<string, SendableModuleClassWrapper> = new collections.Map();
  private static TAG: string = 'SendableLynxModuleManager';

  registerModule(name: string, wrapper: SendableModuleClassWrapper) {
    if (this.modulesMap.has(name)) {
      LLog.e(
        SendableLynxModuleManager.TAG,
        `Duplicated LynxModule For Name: ${name}, will be override`
      );
    }
    this.modulesMap.set(name, wrapper);
    LLog.d(SendableLynxModuleManager.TAG, `registered module with name: ${name}`);
  }

  getModulesMap() {
    return this.modulesMap;
  }

  getModule(name: string): SendableLynxModule | undefined {
    if (name == undefined || name.length == 0) {
      LLog.e(SendableLynxModuleManager.TAG, 'getModule failed, name is null');
      return;
    }
    if (this.modulesByName.get(name) != undefined) {
      return this.modulesByName.get(name);
    }

    const wrapper = this.modulesMap.get(name);
    if (!wrapper) {
      return;
    }
    let module = constructSendableModule(wrapper.moduleClass as Function, wrapper.param) as SendableLynxModule;
    this.modulesByName.set(name, module);
    return module;
  }
}

export function convertModuleArgs(moduleManager: LynxModuleManager | SendableLynxModuleManager) {
  const values = Array.from(moduleManager.getModulesMap().values());
  return [
    moduleManager,
    moduleManager.getModule,
    Array.from(moduleManager.getModulesMap().keys()),
    values.map((item) => {
      return Object.getOwnPropertyNames((item.moduleClass as Function).prototype);
    }),
    values.map((item) => {
      return (item.moduleClass as Record<string, Object>)['syncMethods'] as string[] | undefined;
    })
  ];
}
