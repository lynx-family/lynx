// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import {
  AppProxyParams,
  EnvKey,
  LifeEvent,
  loadCardParams,
  NativeApp,
  requireParamObj,
} from './interface';
import { AMDFactory, AMDModule } from '../common';
import { createSharedConsole, SharedConsole } from '@lynx-js/runtime-shared';
import {
  Reporter,
  BaseError,
  InternalRuntimeError,
  LynxErrorLevel,
  UserRuntimeError,
} from '../modules/report';
import { ContextProxyType, Lynx, NativeLynxProxy } from '../lynx';
import EventEmitter, { AopManager, BeforePublishEvent } from '../modules/event';
import {
  ExposureManager,
  IntersectionObserverManager,
  NativeLynxUIModule,
  NativeModule,
  TextInfo,
  TextInfoManager,
  TextMetrics,
} from '../modules/nativeModules';
import { DEFAULT_ENTRY, SOURCE_MAP_RELEASE_ERROR_NAME } from '../common';
import nativeGlobal from '../common/nativeGlobal';
import {
  CreateIntersectionObserverFunc,
  LynxClearTimeout,
  LynxSetTimeout,
} from '@lynx-js/types';
import Performance from '../modules/performance';
import { reportError } from '../modules/report';
import LynxJSBI from '../common/jsbi';
import { BaseAppSingletonData } from '../standalone/StandaloneApp';
import { CachedFunctionProxy } from '../util/cachedFunctionProxy';
import { getPromiseMaybePolyfill } from '../util/setup-promise';
import { createResponseClass, createRequestClass } from '../modules/fetch';
import { MessageEventType, MessageEvent } from '../lynx';

export abstract class BaseApp<
  NativeAppProxy extends NativeApp = NativeApp,
  LynxImpl extends Lynx = Lynx
> {
  _nativeApp: NativeAppProxy;
  nativeAppId: string;
  _params: loadCardParams;
  lynx: LynxImpl;
  modules: Record<string, Record<string, AMDModule>>;
  sharedConsole: SharedConsole;
  dynamicComponentExports: object;
  loadedDynamicComponentsSet: Set<string>;
  resolvedPromise: Promise<void>;

  Reporter: Reporter;
  _lazyCallableModules: Map<string, unknown>;
  GlobalEventEmitter: EventEmitter;
  NativeModules: NativeModule;
  LynxUIMethodModule: NativeLynxUIModule;
  LynxTestModule: object;
  LynxResourceModule: object;
  LynxAccessibilityModule: object;
  LynxSetModule: object;

  _apiList: Record<string, unknown>;
  _intersectionObserverManager: IntersectionObserverManager;
  _exposureManager: ExposureManager;
  _textInfoManager: TextInfoManager;
  _aopManager: AopManager;
  beforePublishEvent: BeforePublishEvent;

  performance: Performance;

  setTimeout: LynxSetTimeout;
  setInterval: LynxSetTimeout;
  clearInterval: (intervalId: number) => void;
  clearTimeout: (timeoutId: number) => void;

  _createResponseClass: (
    Promise: PromiseConstructor
  ) => ReturnType<typeof createResponseClass>;
  _createRequestClass: (
    Promise: PromiseConstructor
  ) => ReturnType<typeof createRequestClass>;
  _ResponseClass: ReturnType<typeof createResponseClass>;
  _RequestClass: ReturnType<typeof createRequestClass>;

  dataTypeSet = new Set([
    'string',
    'number',
    'array',
    'object',
    'boolean',
    'null',
    'function',
  ]);

  /**
   * Internal Event Listener
   * @private
   */
  private contextProxyTypeToMethod: {};
  private removeInternalEventListenersCallbacks: (() => void)[] = [];

  constructor(
    options: AppProxyParams<NativeAppProxy>,
    baseAppSingleData?: BaseAppSingletonData<NativeAppProxy, LynxImpl>
  ) {
    this.initBase(options);
    if (baseAppSingleData) {
      baseAppSingleData.transferSingletonData(
        this,
        this.__internal__callLynxSetModule.bind(this)
      );
    } else {
      this.initExtra(options);
    }

    // init timeout function
    this.setTimeout = this.nativeApp.setTimeout;
    this.setInterval = this.nativeApp.setInterval;
    this.clearInterval = this.nativeApp.clearInterval;
    this.clearTimeout = this.nativeApp.clearTimeout;

    this.addInternalEventListeners();
  }

  protected initExtra(options: AppProxyParams<NativeAppProxy>) {
    const { lynx } = options;

    this.modules = {};
    this._lazyCallableModules = new Map();
    this._nativeApp = CachedFunctionProxy.create<NativeAppProxy>(
      this._nativeApp
    );
    this.sharedConsole = createSharedConsole(`runtimeId:${this.nativeAppId}`);
    this.dynamicComponentExports = {};
    this.loadedDynamicComponentsSet = new Set();
    this._lazyCallableModules = new Map();

    this.Reporter = new Reporter(
      () => this,
      () => this.nativeApp
    );

    // init eventEmitter
    this.GlobalEventEmitter = new EventEmitter(
      this.__internal__callLynxSetModule.bind(this)
    );
    this._intersectionObserverManager = new IntersectionObserverManager(
      this.NativeModules
    );

    this._exposureManager = new ExposureManager(this.NativeModules);
    this.setupExposureApi();
    this._aopManager = new AopManager();
    this.beforePublishEvent = this._aopManager._beforePublishEvent;

    this.performance = new Performance(this.GlobalEventEmitter, this.nativeApp);

    const promiseCtor = this.setupPromise(
      this.nativeApp.setTimeout,
      this.nativeApp.clearTimeout,
      lynx
    );

    this.lynx = this.createLynx(lynx, promiseCtor);
    this.setupJSModule();
    this.setupIntersectionApi();
    this.setupFetchAPI(promiseCtor);
  }

  protected initBase(options: AppProxyParams<NativeAppProxy>) {
    const { nativeApp, params } = options;

    // init id & loadCardParam
    this.nativeAppId = nativeApp.id;
    this._params = params;
    this._nativeApp = nativeApp;

    // init native NativeModules
    this.NativeModules = nativeApp.nativeModuleProxy;
    this.LynxUIMethodModule = nativeApp.nativeModuleProxy.LynxUIMethodModule;
    this.LynxTestModule = nativeApp.nativeModuleProxy.LynxTestModule;
    this.LynxResourceModule = nativeApp.nativeModuleProxy.LynxResourceModule;
    this.LynxAccessibilityModule =
      nativeApp.nativeModuleProxy.LynxAccessibilityModule;
    this.LynxSetModule = nativeApp.nativeModuleProxy.LynxSetModule;

    //init appList
    this._apiList = {};
    this._textInfoManager = new TextInfoManager(this.NativeModules);
    this.setupGetTextInfoApi();
  }

  static kDefaultSourceMapURL = 'default';
  static kGetSourceMapReleaseErrorName = SOURCE_MAP_RELEASE_ERROR_NAME;
  /**
   * legacy sourcemap release use url default
   * used for backward compatibility
   *
   * new template should use setSourceMapRelease
   */
  set __sourcemap__release__(release: string) {
    let error = new Error();
    error.name = 'LynxGetSourceMapReleaseError';
    error.message = release;
    error.stack = `at <anonymous> (${BaseApp.kDefaultSourceMapURL}:1:1)`;
    this.setSourceMapRelease(error);
  }

  /**
   * Set sourcemap release with a newly thrown error
   * @param {Error} error
   * The error thrown from the file that wants to set sourcemap release.
   * The top frame of `error.stack` **must be** the filename.
   * The `error.name` **must be** `'LynxGetSourceMapReleaseError'`.
   * The `error.message` **must be** the sourcemap release.
   *
   * @example
   * (function () {
   *   try {
   *     throw new Error(sourcemapRelease);
   *   } catch (e) {
   *     e.name = 'LynxGetSourceMapReleaseError';
   *     tt.setSourceMapRelease(e);
   *   }
   * })()
   */
  setSourceMapRelease = (error: Error) => {
    this.Reporter.setSourceMapRelease(error);
  };

  getSourceMapRelease = (url: string): string => {
    return this.Reporter.getSourceMapRelease(url);
  };

  destroy() {
    this.__removeInternalEventListeners();
    this._nativeApp = null;
    this._params = null;
    this._lazyCallableModules = null;
    this.GlobalEventEmitter = null;
  }

  registerModule(name: string, module: object): void {
    this._lazyCallableModules[name] = module;
  }

  getJSModule<Module = unknown>(name: string): Module {
    return this._lazyCallableModules[name];
  }

  setupJSModule() {
    this.registerModule('GlobalEventEmitter', this.GlobalEventEmitter);
    this.registerModule('Reporter', this.Reporter);
  }

  setupFetchAPI(Promise: PromiseConstructor) {
    this._createResponseClass = createResponseClass;
    this._createRequestClass = createRequestClass;
    this._RequestClass = nativeGlobal.Request ?? createRequestClass(Promise);
    this._ResponseClass = nativeGlobal.Response ?? createResponseClass(Promise);

    if (!nativeGlobal.Request) {
      nativeGlobal.Request = this._RequestClass;
    }
    if (!nativeGlobal.Response) {
      nativeGlobal.Response = this._ResponseClass;
    }
  }

  private __internal__callLynxSetModule(functionName: string, payload: any[]) {
    const nativeFunction = this.LynxSetModule[functionName];
    if (nativeFunction) {
      Function.prototype.apply.call(nativeFunction, undefined, payload);
    }
  }

  get nativeApp(): NativeAppProxy {
    return this._nativeApp;
  }

  set nativeApp(nativeApp: NativeAppProxy) {
    this._nativeApp = nativeApp;
  }

  get params(): loadCardParams {
    return this._params;
  }

  set apiList(api: object) {
    this._apiList = { ...this._apiList, ...api };
  }

  setupIntersectionApi() {
    let self = this;
    this._apiList['createIntersectionObserver'] = function (
      component: { componentId: string } & { [key: string]: any },
      options?: {
        thresholds?: [];
        initialRatio?: number;
        observeAll?: boolean;
      }
    ) {
      const { componentId = '' } = component;
      return self._intersectionObserverManager.createIntersectionObserver(
        componentId,
        options
      );
    };
    this.lynx['createIntersectionObserver'] = this._apiList[
      'createIntersectionObserver'
    ] as CreateIntersectionObserverFunc;
  }

  onIntersectionObserverEvent(
    observerId: number,
    callbackId: number,
    data: Record<any, any>
  ): void {
    const observer = this._intersectionObserverManager.getObserver(observerId);
    if (observer) {
      observer.invokeCallback(callbackId, data);
    }
  }

  setupGetTextInfoApi = (): void => {
    this._apiList['getTextInfo'] = (
      text: String,
      options?: TextInfo
    ): TextMetrics => {
      return this._textInfoManager.getTextInfo(text, options);
    };
  };

  setupExposureApi = (): void => {
    this._apiList['resumeExposure'] = (): void => {
      this._exposureManager.resumeExposure();
    };
    this._apiList['stopExposure'] = (options?: {
      sendEvent?: boolean;
    }): void => {
      this._exposureManager.stopExposure(
        options ? options : { sendEvent: true }
      );
    };
    this._apiList['setObserverFrameRate'] = (options?: {
      forPageRect?: number;
      forExposureCheck?: number;
    }): void => {
      this._exposureManager.setObserverFrameRate(
        options ? options : { forPageRect: 20, forExposureCheck: 20 }
      );
    };
  };

  reportError(error: Error) {
    return this.lynx.reportError(error);
  }

  handleError(
    error: BaseError,
    originError?: Error,
    errorLevel?: LynxErrorLevel
  ) {
    reportError(error, this.nativeApp, {
      originError,
      getSourceMapRelease: this.getSourceMapRelease,
      errorLevel: errorLevel,
    });
  }

  handleUserError(
    error?: Error,
    cause?: unknown,
    errorLevel?: LynxErrorLevel,
    prefix?: string
  ): void {
    let { message, name, stack } = error || {};
    if (!message) {
      // If there is no error message in error, means that it is not an error-like object.
      // We construct a new Error using JSON.stringify
      ({ message, name, stack } = new Error(JSON.stringify(error)));
    }
    const userError = new UserRuntimeError(
      prefix ? `${prefix} ${name}: ${message}` : `${name}: ${message}`,
      stack
    );
    userError.cause = cause;
    this.handleError(userError, error, errorLevel);
  }

  /**
   * @internal
   */
  handleInternalError(error?: Error, cause?: unknown): void {
    let { message, name, stack } = error || {};
    if (!message) {
      // If there is no error message in error, means that it is not an error-like object.
      // We construct a new Error using JSON.stringify
      ({ message, name, stack } = new Error(JSON.stringify(error)));
    }
    const internalError = new InternalRuntimeError(
      `${name}: ${message}`,
      stack
    );
    internalError.cause = cause;
    this.handleError(internalError, error);
  }

  /**
   * Get a external env with boolean value.
   * The same as `base::LynxEnv::GetInstance().GetBoolEnv`
   *
   * @param {EnvKey} key The {@link EnvKey}, should be placed in `lynx_env.h`
   */
  getBoolEnv(key: EnvKey): boolean {
    const env = this.nativeApp.getEnv(key);
    return env?.toLowerCase() === 'true';
  }

  /**
   * @internal
   * @static
   * The LynxGroup level cache for requireModule , {@link registerModule}
   */
  static _$factoryCache: Record<
    string,
    <T>(injected: { tt: BaseApp }) => T
  > = {};

  /**
   * @internal
   * Execute the loaded JS module ,  Called by {@link requireModule} & {@link requireModuleAsync}
   * @throws {UserRuntimeError} when loading or evaluating failed
   * @throws {Error} when executing failed
   */
  private _$executeInit<T>(
    exports: ReturnType<NativeApp['loadScript']>,
    { path, entryName }: { path: string; entryName?: string }
  ): T {
    let factory: <T>(injected: { tt: BaseApp }) => T;
    if (exports && exports.init) {
      // app-service.js and common-chunk.js with new format will have init function
      factory = exports.init.bind(exports);
    } else if (nativeGlobal.initBundle) {
      // common-chunk.js with old format will set global.initBundle during loadScript
      factory = nativeGlobal.initBundle.bind(nativeGlobal.initBundle);
      delete nativeGlobal.initBundle; // should delete initBundle after used
    } else {
      // no factory function found, probably loadScript failed.
      // TODO(wangqingyu): do not throw this when `nativeApp.loadScript` support exceptions
      throw new UserRuntimeError(
        `load failed. path:${path},entryName:${entryName}`
      );
    }
    try {
      nativeConsole.profile(`running ${path} init`);
      const ret = factory<T>({ tt: this });

      // Here means that no error occured when executing.
      // Only then we cache the factory.
      BaseApp._$factoryCache[path] = factory;
      return ret;
    } finally {
      nativeConsole.profileEnd();
    }
  }

  /**
   * @internal
   * Used to load the json module. Called by {@link requireModule} & {@link requireModuleAsync}
   * @param content
   * @param path
   * @private
   */
  private _$executeJSON<T>(
    content: ReturnType<NativeApp['readScript']>,
    { path }: { path: string; entryName?: string }
  ): T {
    const ret = JSON.parse(content);
    const init = () => ret;
    BaseApp._$factoryCache[path] = init;
    return ret;
  }

  requireModule<T>(
    path: string,
    entryName?: string,
    options?: { timeout: number }
  ): T {
    const init = BaseApp._$factoryCache[path];
    if (NODE_ENV !== 'development' && init) {
      // cache hit
      return this._$executeInit<T>({ init }, { path, entryName });
    }

    // cache miss
    if (path.split('?')[0].endsWith('.json')) {
      const content = this.nativeApp.readScript(path, {
        dynamicComponentEntry: entryName ?? DEFAULT_ENTRY,
        ...options,
      });
      return this._$executeJSON(content, { path, entryName });
    }

    const exports = this.nativeApp.loadScript(path, entryName, options);
    return this._$executeInit<T>(exports, { path, entryName });
  }

  requireModuleAsync<T>(
    path: string,
    callback: (error?: Error, exports?: T) => void
  ): void {
    const init = BaseApp._$factoryCache[path];
    if (NODE_ENV !== 'development' && init) {
      // cache hit
      callback(null, this._$executeInit<T>({ init }, { path }));
      return;
    }
    // cache miss
    if (path.split('?')[0].endsWith('.json')) {
      const content = this.nativeApp.readScript(path);
      const ret = this._$executeJSON<T>(content, { path });
      callback(null, ret);
      return;
    }

    // Create an error here to make sure the stack contains
    // lynx.requireModuleAsync and it's caller.
    const error = new Error();
    this.nativeApp.loadScriptAsync(path, (message, exports): void => {
      if (message) {
        error.message = message;
        // Only override error.message so that we could privide stack with
        // lynx.requireModuleAsync and it's caller.
        return callback(error);
      }

      try {
        return callback(null, this._$executeInit(exports, { path }));
      } catch (e) {
        return callback(e);
      }
    });
  }

  require(path: string, params?: requireParamObj) {
    const that = this;
    if (typeof path !== 'string') {
      throw new Error('require args must be a string');
    }
    const entryName =
      params && params.dynamicComponentEntry
        ? params.dynamicComponentEntry
        : DEFAULT_ENTRY;
    if (!that.modules[entryName]) {
      that.modules[entryName] = {};
    }
    let module = that.modules[entryName][path];
    if (!module) {
      try {
        // eslint-disable-next-line @typescript-eslint/no-unused-vars
        const tt = that;
        const jsContent = that._nativeApp.readScript(path, {
          dynamicComponentEntry: entryName,
        });
        // eslint-disable-next-line no-eval
        eval(jsContent);
        module = that.modules[entryName][path];
      } catch (e) {
        this.handleError(
          new UserRuntimeError(
            `eval user: ${that._nativeApp.id} error: ${e.message}`,
            e.stack
          ),
          e
        );
      }

      if (!that.modules[entryName][path]) {
        throw new Error(
          `module ${path} in ${entryName} is not defined in card: ${that._nativeApp.id}`
        );
      }
    }

    if (!module.hasRun) {
      const { factory } = module;
      const _module = {
        exports: {},
      };
      let res;

      module.hasRun = true;
      module.exports = _module.exports;
      if (typeof factory === 'function') {
        const inRequireCopy = inRequire.call(that, path);
        const tt = that;
        res = factory(
          inRequireCopy,
          _module,
          _module.exports,
          that.Card.bind(tt),
          that.setTimeout,
          that.setInterval,
          that.clearInterval,
          that.clearTimeout,
          that.NativeModules,
          that._apiList,
          that.sharedConsole,
          that.Component.bind(tt),
          params?.ReactLynx,
          that.nativeAppId,
          that.Behavior.bind(tt),
          LynxJSBI,
          that.lynx,
          undefined, // window
          undefined, // document
          undefined, // frames
          undefined, // self
          undefined, // location
          undefined, // navigator
          undefined, // localStorage
          undefined, // history
          undefined, // Caches
          undefined, // screen
          undefined, // alert
          undefined, // confirm
          undefined, // prompt
          that.lynx.fetch, // fetch
          undefined, // XMLHttpRequest
          undefined, // WebSocket
          undefined, // webkit
          undefined, // Reporter
          undefined, // print
          undefined, // global
          that.requestAnimationFrame,
          that.cancelAnimationFrame
        );
        module.exports = _module.exports || res;
      }
    }
    return module.exports;
  }

  define(path: string, factory: AMDFactory, entryName?: string) {
    entryName = entryName ? entryName : DEFAULT_ENTRY;
    if (!this.modules[entryName]) {
      this.modules[entryName] = {};
    }
    this.modules[entryName][path] = {
      hasRun: false,
      factory: factory.bind(this),
    };
  }

  /**
   * Call By Native js_app
   * @internal
   * @param module
   * @param method
   * @param args
   */
  callFunction(module: string, method: string, args?: unknown[]): void {
    try {
      const moduleMethods = this.getJSModule(module);
      if (typeof moduleMethods[method] === 'function') {
        moduleMethods[method].apply(moduleMethods, args);
      }
    } catch (e) {
      this.handleUserError(e, { by: `${module}.${method}` });
    }
  }

  /**
   * Call By Native js_app
   * @internal
   * @param {never} _ Used for backward compatiblity, DO NOT USE.
   * @param {Error} error the Error object emit by native.
   */
  onAppError(_: never, error: Error): void {
    this.handleInternalError(error);
  }

  saveDynamicComponentExports(componentUrl, moduleExports) {
    this.dynamicComponentExports[componentUrl] = moduleExports;
  }

  getDynamicComponentExports(componentUrl) {
    return this.dynamicComponentExports[componentUrl];
  }

  Component(...args: unknown[]): void {}

  Card(...args: unknown[]): void {}

  Behavior?(...args: unknown[]): void {}

  /**
   * @param setTimeout
   */
  wrapReport(setTimeout: Function, desc: string) {
    const that = this;

    function wrapReport(fn: Function) {
      return function wrapReportInner(...args: any[]) {
        try {
          return fn.apply(this, args);
        } catch (e) {
          that.handleUserError(e, { by: desc });
        }
      };
    }

    return function WrapTimeout(fn: Function, ...args: any[]) {
      return Function.prototype.apply.call(setTimeout, undefined, [
        wrapReport(fn),
        ...args,
      ]);
    };
  }

  setupPromise(
    setTimeout: LynxSetTimeout,
    clearTimeout: LynxClearTimeout,
    lynx: NativeLynxProxy
  ) {
    const PromiseConstructor = getPromiseMaybePolyfill(
      setTimeout,
      (id, reason: Error) => {
        try {
          if (reason) {
            if (!reason.stack) {
              reason = new Error(JSON.stringify(reason));
            }
            reason.name = 'unhandled rejection';
            this.handleUserError(reason);
          }
        } catch (err) {
          // just ignore
        }
      },
      clearTimeout,
      lynx.queueMicrotask,
      this._params?.pageConfigSubset?.enableMicrotaskPromisePolyfill ?? false
    );
    this.resolvedPromise = PromiseConstructor.resolve();
    return PromiseConstructor;
  }

  requestAnimationFrame = (callback: () => void) =>
    this._nativeApp.requestAnimationFrame(callback);

  cancelAnimationFrame = (animationId: number) =>
    this._nativeApp.cancelAnimationFrame(animationId);

  protected addInternalEventListener(
    contextProxyType: ContextProxyType,
    type: string,
    listener: (event: MessageEvent) => void
  ) {
    this.contextProxyTypeToMethod[contextProxyType]().addEventListener(
      type,
      listener
    );
    this.removeInternalEventListenersCallbacks.push(() => {
      this.contextProxyTypeToMethod[contextProxyType]().removeEventListener(
        type,
        listener
      );
    });
  }

  protected addInternalEventListeners() {
    if (!this.contextProxyTypeToMethod) {
      this.contextProxyTypeToMethod = {
        [ContextProxyType.CoreContext]: () => this.lynx.getCoreContext(),
        [ContextProxyType.DevTool]: () => this.lynx.getDevtool(),
        [ContextProxyType.JSContext]: () => this.lynx.getJSContext(),
        [ContextProxyType.UIContext]: () => this.lynx.getUIContext(),
      };
    }

    this.addInternalEventListener(
      ContextProxyType.CoreContext,
      MessageEventType.ON_NATIVE_APP_READY,
      () => {
        this.onNativeAppReady();
      }
    );
    this.addInternalEventListener(
      ContextProxyType.CoreContext,
      MessageEventType.NOTIFY_GLOBAL_PROPS_UPDATED,
      (event: MessageEvent) => {
        this.updateGlobalProps(event.data);
      }
    );
    this.addInternalEventListener(
      ContextProxyType.CoreContext,
      MessageEventType.ON_LIFECYCLE_EVENT,
      (event: MessageEvent) => {
        this.OnLifecycleEvent(event.data);
      }
    );
    this.addInternalEventListener(
      ContextProxyType.CoreContext,
      MessageEventType.ON_APP_FIRST_SCREEN,
      () => {
        this.onAppFirstScreen();
      }
    );
    this.addInternalEventListener(
      ContextProxyType.CoreContext,
      MessageEventType.ON_DYNAMIC_JS_SOURCE_PREPARED,
      (event: MessageEvent) => {
        nativeGlobal.loadDynamicComponent(this, event.data);
      }
    );
    this.addInternalEventListener(
      ContextProxyType.CoreContext,
      MessageEventType.ON_APP_ENTER_FOREGROUND,
      () => {
        this.onAppEnterForeground();
      }
    );
    this.addInternalEventListener(
      ContextProxyType.CoreContext,
      MessageEventType.ON_APP_ENTER_BACKGROUND,
      () => {
        this.onAppEnterBackground();
      }
    );
  }

  private __removeInternalEventListeners = () => {
    this.removeInternalEventListenersCallbacks.forEach((f) => {
      f();
    });
  };

  /**
   *  override by subclass
   * @param newData
   */
  updateGlobalProps(newData: object): void {}

  /**
   *  override by subclass
   * @param newData
   */
  OnLifecycleEvent(
    args: [
      string,
      LifeEvent | LifeEvent[],
      {
        props?: Record<string, unknown>;
        initData?: Record<string, unknown>;
        dataset?: Record<string, string>;
        id?: string;
        className?: string;
        parentId?: string;
        path?: string;
        entryName?: string;
        /**
         * additional arguments like forceFlush for SSR can be put here
         */
        [key: string]: unknown;
      }
    ]
  ): void {}

  /**
   *  override by subclass
   * @param newData
   */
  onNativeAppReady(): void {}

  /**
   *  override by subclass
   * @param newData
   */
  onAppFirstScreen(): void {}

  /**
   *  override by subclass
   * @param newData
   */
  onAppEnterBackground(): void {}

  /**
   *  override by subclass
   * @param newData
   */
  onAppEnterForeground(): void {}

  abstract createLynx(
    nativeLynx: NativeLynxProxy,
    promiseCtor: PromiseConstructor
  ): LynxImpl;
}

function pathProcess(path: string): string {
  const match = path.match(/(.*)\/([^/]+)?$/);
  return match?.[1] ? match[1] : './';
}

function inRequire(path: string): Function {
  const that = this;
  const pwd = pathProcess(path);

  return function (path) {
    const t = [];
    const r = `${pwd}/${path}`.split('/');
    const i = r.length;

    if (typeof path !== 'string') {
      throw new Error('require args must be a string');
    }
    for (let o = 0; o < i; ++o) {
      const a = r[o];
      if (a !== '' && a !== '.') {
        if (a === '..') {
          if (t.length === 0) {
            throw new Error(
              `can't find module ${path} in app: ${that._nativeApp.id}`
            );
          }
          t.pop();
        } else {
          o + 1 < i && r[o + 1] === '..' ? o++ : t.push(a);
        }
      }
    }
    let c = t.join('/');
    /* eslint-disable no-return-assign */
    /* eslint-disable no-sequences */
    return c.endsWith('.js') || (c += '.js'), that.require(c);
  };
}
