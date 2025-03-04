import {
  isError,
  isFunction,
  isObject,
  isString,
} from '@lynx-js/runtime-shared';
import {
  CreateIntersectionObserverFunc,
  GlobalProps,
  LoadDynamicComponentFailedResult,
  LoadDynamicComponentFunc,
  LoadDynamicComponentSuccessResult,
  LynxSetTimeout,
} from '@lynx-js/types';
import {
  RequireModule,
  RequireModuleAsync,
  DispatchEventResult,
  NativeLynxProxy,
} from './interface';
import { BaseApp, NativeApp } from '../app';
import { TextInfo, TextMetrics } from '../modules/nativeModules';
import nativeGlobal from '../common/nativeGlobal';
import Element from '../modules/element';
import { LynxErrorLevel } from '../modules/report';
import { MessageEventType, MessageEvent } from './interface';
import Performance from '../modules/performance';
import SelectorQuery from '../modules/selectorQuery/SelectorQuery';

export class Lynx {
  static __registerSharedDataCounter: number = 0;
  __globalProps: GlobalProps;
  __presetData: Record<string, unknown>;
  _switches: Record<string, boolean>;
  targetSdkVersion?: string;

  constructor(
    // should use function to get native app to avoid cycle
    public getNativeApp: () => NativeApp,
    public getApp: () => BaseApp,
    public Promise: PromiseConstructor,
    public getNativeLynx: () => NativeLynxProxy
  ) {
    this.init(undefined);
  }

  setTimeout: LynxSetTimeout = this.getApp().wrapReport(
    this.getNativeApp().setTimeout,
    'setTimeout Error'
  );

  public rebind(getApp: () => BaseApp) {
    this.init(getApp);
  }

  private init(getApp?: () => BaseApp) {
    if (getApp) {
      this.getApp = getApp;
      // TODO(liyanbo): merge or replace? now is replace.
      this.__globalProps = this.getNativeLynx().__globalProps || {};
      this.__presetData = this.getNativeLynx().__presetData || {};
    } else {
      const cache = {};
      this.requireModule.cache = cache;
      this.requireModuleAsync.cache = cache;
      this.__globalProps = this.getNativeLynx().__globalProps || {};
      this.__presetData = this.getNativeLynx().__presetData || {};
      this._switches = {};
    }
  }

  setInterval: LynxSetTimeout = this.getApp().wrapReport(
    this.getNativeApp().setInterval,
    'setInterval Error'
  );
  clearInterval = this.getNativeApp().clearInterval;
  clearTimeout = this.getNativeApp().clearTimeout;

  resumeExposure = this.getApp()._apiList['resumeExposure'] as () => void;

  requireModule = <RequireModule>(<T>(path: string, entryName?: string): T => {
    if (this.requireModule.cache[path]) {
      return this.requireModule.cache[path] as T;
    }
    // TODO(wangqingyu): deal with cyclic requireModule
    const exports = this.getApp().requireModule<T>(path, entryName);

    // When error happens in loading or executing, a JS error will be thrown.
    // So when we are here, the module is loaded and executed successfully.
    this.requireModule.cache[path] = exports;
    return exports;
  });

  requireModuleAsync = <RequireModuleAsync>(<T>(
    path: string,
    callback?: (error?: Error, ret?: T) => void
  ): void => {
    callback ??= (error?: Error) => {
      if (!error) {
        // `undefined | null` means no error occurred
        return;
      }
      this.getApp().handleUserError(error);
    };

    if (this.requireModuleAsync.cache[path]) {
      callback(null, this.requireModuleAsync.cache[path] as T);
      return;
    }
    // TODO(wangqingyu): deal with cyclic requireModule
    this.getApp().requireModuleAsync<T>(path, (error, exports) => {
      if (!error) {
        // Only cache the exports when no error happends.
        this.requireModuleAsync.cache[path] = exports;
      }
      callback(error, exports);
    });
  });

  createElement = (rootId: string, id: string) =>
    this.getNativeLynx().createElement(rootId, id);

  getElementById = (id: string): Element => {
    return new Element('', id, this);
  };

  reportError = (error: Error | string, options?: { level?: string }): void => {
    let errorObj: Error;
    if (isError(error)) {
      errorObj = error;
    } else {
      let message: string;
      if (typeof error !== 'string') {
        message = JSON.stringify(error);
      } else {
        message = error;
      }
      errorObj = new Error(message);
    }
    const { level = 'error' } = options || {};
    let errorLevel: LynxErrorLevel;
    switch (level) {
      case 'error':
        errorLevel = LynxErrorLevel.Error;
        break;
      case 'warning':
        errorLevel = LynxErrorLevel.Warn;
        break;
      case 'fatal':
        errorLevel = LynxErrorLevel.Fatal;
        break;
      default:
        errorLevel = LynxErrorLevel.Error;
    }
    this.getApp().handleUserError(errorObj, undefined, errorLevel);
  };

  registerModule = <Module extends object>(
    name: string,
    module: Module
  ): void => this.getApp().registerModule(name, module);

  getJSModule = <Module = unknown>(name: string): Module => {
    return this.getApp().getJSModule<Module>(name);
  };

  getTextInfo = this.getApp()._apiList['getTextInfo'] as (
    text: string,
    info: TextInfo
  ) => TextMetrics;

  addFont = (
    font: { src: string; 'font-family': string },
    callback: (e?: Error) => void
  ) => {
    if (!isObject(font)) {
      throw new Error('The first argument must be object type');
    }
    if (!isString(font['font-family']) || !isString(font['src'])) {
      throw new Error('The font value must have font-family and src');
    }
    if (!isFunction(callback)) {
      throw new Error('The second argument must be function type');
    }

    this.getNativeLynx().addFont(font, callback);
  };

  stopExposure = this.getApp()._apiList['stopExposure'] as (options?: {
    sendEvent: true;
  }) => void;

  setObserverFrameRate = this.getApp()._apiList[
    'setObserverFrameRate'
  ] as (options?: { forPageRect?: number; forExposureCheck?: number }) => void;

  performance: Performance = this.getApp().performance;

  beforePublishEvent = this.getApp()._aopManager._beforePublishEvent;

  dispatchSessionStorageEvent(event: MessageEvent): void {
    var eventResult = this.getCoreContext().dispatchEvent(event);

    // In LynxView, the event has been sucessfully handled by `CoreContext`.
    if (eventResult == DispatchEventResult.NotCanceled) {
      return;
    }

    // In runtime standalone mode, runtime cannot dispatch event to `CoreContext`,
    // fallback to `JSContext` so that runtime can handle session storage events
    // by itself.
    this.getJSContext().dispatchEvent(event);
  }

  // sessionStorage Api
  setSessionStorageItem = <T>(key: string, value: T): void => {
    this.dispatchSessionStorageEvent({
      type: MessageEventType.EVENT_SET_SESSION_STORAGE,
      data: {
        key,
        value,
      },
    });
  };

  getSessionStorageItem = <T>(
    key: string,
    callback: (value: T) => void
  ): void => {
    // TODO(nihao.royal): refactor to dispatchEvent after ApiCallback supported.
    this.getNativeApp().getSessionStorageItem(key, callback);
  };

  subscribeSessionStorage = <T>(
    key: string,
    callback: (value: T) => void
  ): number => {
    // TODO(nihao.royal): refactor to dispatchEvent after ApiCallback supported.
    let listenerId = Lynx.__registerSharedDataCounter++;
    this.getNativeApp().subscribeSessionStorage(key, listenerId, callback);
    return listenerId;
  };

  unsubscribeSessionStorage = (key: string, listenerId: number) => {
    this.dispatchSessionStorageEvent({
      type: MessageEventType.EVENT_UNSUBSCRIBE_SESSION_STORAGE,
      data: {
        key,
        listenerId,
      },
    });
  };

  getDevtool = this.getNativeLynx().getDevtool;
  getCoreContext = this.getNativeLynx().getCoreContext;
  getJSContext = this.getNativeLynx().getJSContext;
  getUIContext = this.getNativeLynx().getUIContext;

  getCustomSectionSync = this.getNativeLynx().getCustomSectionSync;

  accessibilityAnnounce = this.getNativeApp().nativeModuleProxy
    .LynxAccessibilityModule?.accessibilityAnnounce;

  requestResourcePrefetch = this.getNativeApp().nativeModuleProxy
    .LynxResourceModule?.requestResourcePrefetch;

  cancelResourcePrefetch = this.getNativeApp().nativeModuleProxy
    .LynxResourceModule?.cancelResourcePrefetch;

  setSharedData = (dataKey: string, dataVal: unknown): void => {
    nativeGlobal.sharedData[dataKey] = dataVal;
    let variable = {};
    variable[dataKey] = dataVal;
    nativeGlobal.shareDataSubject.notifyDataChange(variable);
  };

  getSharedData = <T = unknown>(dataKey: string): T =>
    nativeGlobal.sharedData[dataKey];

  registerSharedDataObserver = <T>(callback: (data: T) => void): void =>
    nativeGlobal.shareDataSubject.registerObserver(callback);

  removeSharedDataObserver = <T>(callback: (data: T) => void): void =>
    nativeGlobal.shareDataSubject.removeObserver(callback);

  triggerLepusGlobalEvent = (event: string, params: Record<any, any>): void =>
    this.getNativeApp().triggerLepusGlobalEvent(event, params);

  // for reload
  reload = (value: object, callback: () => void) => {
    this.getNativeLynx().reload(value, callback);
  };

  createIntersectionObserver: CreateIntersectionObserverFunc;

  fetchDynamicComponent = (
    url: string,
    options: Record<string, any>,
    callback: (res: { code: number }) => void,
    id: string[]
  ) => this.getNativeLynx().fetchDynamicComponent(url, options, callback, id);

  // Wrapper QueryComponent to decide if component has loaded.
  QueryComponent = (source: string, callback: (result: any) => void) => {
    const innerInvokeCallback = () => {
      callback({
        code: 0,
        data: { url: source, sync: true, error_message: '', mode: 'cache' },
        detail: { schema: source, cache: false, errMsg: '' },
      });
    };
    // if dynamic componet has been ready in background thread, callback directly
    if (this.getApp().loadedDynamicComponentsSet.has(source)) {
      innerInvokeCallback();
      return;
    }
    // if dynamic componet has been ready in main thread, loadDynamicComponent and callback directly
    const innerCallback = (result: any) => {
      if (result.__hasReady === true) {
        nativeGlobal.loadDynamicComponent(this.getApp(), source);
        innerInvokeCallback();
      } else {
        callback(result);
      }
    };
    // query componet
    this.getNativeLynx().QueryComponent(source, innerCallback);
  };

  loadDynamicComponent: LoadDynamicComponentFunc = (
    idOrUrl: string | string[],
    urlOrOptions?: string | Record<string, any>,
    options: Record<string, any> = {}
  ): Promise<
    LoadDynamicComponentSuccessResult | LoadDynamicComponentFailedResult
  > => {
    return new this.Promise((resolve, reject) => {
      // legal param types:
      // 0. (url: string, ?options)
      // 1. (id: string, url: string, ?options)
      // 2. (ids: string[], url: string, ?options)
      let ids: string[] = [];
      let url: string;
      if (Array.isArray(idOrUrl)) {
        ids = idOrUrl;
        url = urlOrOptions as string;
      } else if (typeof urlOrOptions === 'string') {
        ids = [idOrUrl];
        url = urlOrOptions;
      } else {
        url = idOrUrl;
        options = urlOrOptions;
      }
      if (this.getApp().loadedDynamicComponentsSet.has(url)) {
        // invoke directly
        resolve({
          code: 0,
          data: { url: url, sync: false, error_message: '', mode: 'normal' },
          detail: { schema: url, cache: false, errMsg: '' },
        } as LoadDynamicComponentSuccessResult);
        return;
      }

      this.getNativeLynx().fetchDynamicComponent(
        url,
        options,
        (res) => {
          if (res && res.code == 0) {
            resolve(res as LoadDynamicComponentSuccessResult);
          } else {
            reject(res);
          }
        },
        ids
      );
    });
  };

  fetch = (input: RequestInfo, init?: RequestInit): Promise<Response> => {
    return new this.Promise((resolve, reject) => {
      const request = new (this.getApp()._RequestClass)(input, init);
      const signal = request.signal;
      if (signal.aborted) {
        return reject(signal.reason);
      }

      signal.addEventListener('abort', (event) => {
        reject(signal.reason);
      });

      const fetchArg = {
        method: request.method,
        url: request.url,
        origin: this.getNativeApp().__pageUrl,
        headers: Object.fromEntries(request.headers.entries()),
        body: request._bodyData.arrayBuffer,
        lynx: request.lynxExtension,
      };
      this.getApp().NativeModules.LynxFetchModule.fetch(
        fetchArg,
        (response: any) => {
          if (signal.aborted) {
            return;
          }
          try {
            const resp = new (this.getApp()._ResponseClass)(
              response.body,
              response
            );
            resolve(resp);
          } catch (_) {
            // Catches any exception that might lead to a failure in
            // creating a Response and throws the error using `reject`,
            // enabling the frontend to handle the error.
            reject(new TypeError(response.statusText));
          }
        },
        (error: any) => {
          if (signal.aborted) {
            return;
          }
          reject(new TypeError(error.message));
        }
      );
    });
  };

  createSelectorQuery = (): SelectorQuery => {
    return SelectorQuery.newEmptyQuery({
      nativeApp: this.getNativeApp(),
      lynx: this,
    });
  };

  requestAnimationFrame = (callback: () => void) =>
    this.getNativeApp().requestAnimationFrame(callback);

  cancelAnimationFrame = (animationId: number) =>
    this.getNativeApp().cancelAnimationFrame(animationId);

  queueMicrotask(callback: () => void): void {
    this.getNativeLynx().queueMicrotask(callback);
  }

  __addReporterCustomInfo = (info: Record<string, string>): void => {
    this.getNativeApp().__addReporterCustomInfo(info);
  };
}
