// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { DEFAULT_ENTRY } from '../common';
import { SharedConsole } from '@lynx-js/runtime-shared';
import { AppProxyParams, BaseApp, loadCardParams, NativeApp } from '../app';
import { Lynx, NativeLynxProxy } from '../lynx';
import {
  CallLynxSetModule,
  ExposureManager,
  IntersectionObserverManager,
  TextInfoManager,
} from '../modules/nativeModules';
import { AopManager } from '../modules';
import EventEmitter from '../modules/event';
import { Reporter } from '../modules';
import Performance from '../modules/performance';
import { CachedFunctionProxy } from '../util';
import { AMDModule } from '../common/amd';

export class BaseAppSingletonData<
  NativeAppProxy extends NativeApp = NativeApp,
  LynxImpl extends Lynx = Lynx
> {
  nativeApp: NativeAppProxy;
  sharedConsole: SharedConsole;
  dynamicComponentExports: object;
  loadedDynamicComponentsSet: Set<string>;
  intersectionObserverManager: IntersectionObserverManager;
  exposureManager: ExposureManager;
  textInfoManager: TextInfoManager;
  globalEventEmitter: EventEmitter;
  aopManager: AopManager;
  performance: Performance;
  modules: Record<string, Record<string, AMDModule>>;
  lazyCallableModules: Map<string, unknown>;
  lynx: LynxImpl;
  apiList: Record<string, unknown>;
  Reporter: Reporter;
  resolvedPromise: Promise<void>;

  public transferSingletonData(
    baseApp: BaseApp,
    callLynxSetModule?: CallLynxSetModule
  ) {
    baseApp.nativeApp = this.nativeApp;
    baseApp.sharedConsole = this.sharedConsole;
    baseApp.dynamicComponentExports = this.dynamicComponentExports;
    baseApp.loadedDynamicComponentsSet = this.loadedDynamicComponentsSet;
    baseApp._intersectionObserverManager = this.intersectionObserverManager;
    baseApp._exposureManager = this.exposureManager;
    baseApp._textInfoManager = this.textInfoManager;
    this.globalEventEmitter.setCallLynxSetModule(callLynxSetModule);
    baseApp.GlobalEventEmitter = this.globalEventEmitter;
    baseApp._aopManager = this.aopManager;
    baseApp.performance = this.performance;
    baseApp.modules = this.modules;
    baseApp._lazyCallableModules = this.lazyCallableModules;
    baseApp.lynx = this.lynx;
    this.lynx.rebind(() => baseApp);
    baseApp._apiList = this.apiList;
    this.Reporter.rebind(() => baseApp);
    baseApp.Reporter = this.Reporter;
    baseApp.resolvedPromise = this.resolvedPromise;
  }
}

export default class StandaloneApp extends BaseApp {
  public singletonData: BaseAppSingletonData;

  constructor(options: AppProxyParams<NativeApp>, params: loadCardParams) {
    super(options, undefined);
    this.fillSingletonData();
    try {
      if (params.srcName) {
        delete this.lynx.requireModule.cache[params.srcName];
        delete BaseApp._$factoryCache[params.srcName];
        this.lynx.requireModule(params.srcName, DEFAULT_ENTRY);
        this.dataTypeSet.add('undefined');
      }
    } catch (e) {
      this.handleUserError(e);
    }
  }

  createLynx(nativeLynx: NativeLynxProxy, promise: PromiseConstructor): Lynx {
    const lynx_proxy = CachedFunctionProxy.create(nativeLynx);
    return new Lynx(
      () => this.nativeApp,
      () => this,
      promise,
      () => lynx_proxy
    );
  }

  private fillSingletonData() {
    this.singletonData = new BaseAppSingletonData();
    this.singletonData.nativeApp = this._nativeApp;
    this.singletonData.sharedConsole = this.sharedConsole;
    this.singletonData.dynamicComponentExports = this.dynamicComponentExports;
    this.singletonData.loadedDynamicComponentsSet = this.loadedDynamicComponentsSet;
    this.singletonData.intersectionObserverManager = this._intersectionObserverManager;
    this.singletonData.exposureManager = this._exposureManager;
    this.singletonData.textInfoManager = this._textInfoManager;
    this.singletonData.globalEventEmitter = this.GlobalEventEmitter;
    this.singletonData.aopManager = this._aopManager;
    this.singletonData.performance = this.performance;
    this.singletonData.modules = this.modules;
    this.singletonData.lazyCallableModules = this._lazyCallableModules;
    this.singletonData.lynx = this.lynx;
    this.singletonData.apiList = this._apiList;
    this.singletonData.Reporter = this.Reporter;
    this.singletonData.resolvedPromise = this.resolvedPromise;
  }
}
