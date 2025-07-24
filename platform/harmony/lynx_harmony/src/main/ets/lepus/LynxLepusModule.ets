// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
import { LLog } from '../base/LLog';
import { LynxModuleManager } from '../jsbridge/LynxModuleManager';
import { LynxTemplateRenderer } from '../tasm/LynxTemplateRenderer';

export class LynxLepusModule {
  private static readonly TAG = 'LynxLepusModule';
  private static readonly PARAM_METHOD_DETAIL = 'methodDetail';
  private static readonly PARAM_MODULE = 'module';
  private static readonly PARAM_METHOD = 'method';
  private static readonly PARAM_DATA = 'param';
  private static readonly PARAM_ENTRY = 'tasmEntryName';
  private static readonly PARAM_CALLBACKID = 'callbackId';
  private static readonly METHOD_CALL = 'call';
  private static readonly METHOD_CALL_SYNC = 'callSync';
  private static readonly PARAM_FROM_PIPER = 'fromPiper';

  public static triggerLepusBridgeAsync(methodName: string, args: Record<string, Object>,
    renderer: LynxTemplateRenderer,
    moduleManager: LynxModuleManager) {
    LynxLepusModule.invokeLepusModuleInternal(methodName, args, renderer, moduleManager, true)
  }

  public static triggerLepusBridge(methodName: string, args: Record<string, Object>,
    renderer: LynxTemplateRenderer,
    moduleManager: LynxModuleManager): Object | undefined {
    return LynxLepusModule.invokeLepusModuleInternal(methodName, args, renderer, moduleManager, false);
  }

  private static invokeLepusModuleInternal(methodName: string, args: Record<string, Object>,
    renderer: LynxTemplateRenderer, moduleManager: LynxModuleManager, async: boolean) {
    //{
    //  methodDetail: {
    //    module: string,
    //    method: string,
    //    param?: Object[],
    //  },
    //  fromPiper: boolean,
    //  tasmEntryName: string,
    //  callbackId: number,
    //}
    const methodDetailMap = args[LynxLepusModule.PARAM_METHOD_DETAIL] as Record<string, Object>;
    if (!methodDetailMap) {
      return;
    }
    const moduleName = methodDetailMap[LynxLepusModule.PARAM_MODULE] as string;
    if (!moduleName) {
      return;
    }

    const moduleMap = moduleManager.getModulesMap();
    const moduleWrapper = moduleMap.get(moduleName);
    if (!moduleWrapper) {
      return;
    }

    const module = moduleManager.getModule(moduleName);
    if (!module) {
      return;
    }

    const moduleClass = moduleWrapper.moduleClass;
    const method = methodDetailMap[LynxLepusModule.PARAM_METHOD] as string;
    const fromPiper = args[LynxLepusModule.PARAM_FROM_PIPER] || false;
    const entryName = args[LynxLepusModule.PARAM_ENTRY] as string;
    const id = args[LynxLepusModule.PARAM_CALLBACKID] as number ?? -1;
    const isCallMethod = methodName === LynxLepusModule.METHOD_CALL || (method && method.length !== 0);
    const found = Object.getOwnPropertyNames(moduleClass.prototype).indexOf(methodName) !== -1;
    if (!found) {
      return;
    }
    let ret: Object | undefined = undefined;
    try {
      const callback = async ? (arg: Object) => {
        renderer.invokeLepusCallback(id, entryName, arg);
      } : undefined;

      if (isCallMethod) {
        ret = moduleClass.prototype[methodName].call(module, method, methodDetailMap, callback);
      } else if (fromPiper) {
        const paramArray = methodDetailMap[LynxLepusModule.PARAM_DATA] || [];
        ret = moduleClass.prototype[methodName].call(module, paramArray);
      } else {
        ret = moduleClass.prototype[methodName].call(module, methodDetailMap, callback);
      }
    } catch (e) {
      LLog.e(LynxLepusModule.TAG, 'Lepus Module invoke: ' + e.toString());
    }
    return ret;
  }
}
