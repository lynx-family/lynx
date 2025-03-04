// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// start js app, native has decode js code;
// return means loadCard success or failed.
import { BaseApp, loadCardParams, NativeApp } from './app';
import { Lynx, NativeLynxProxy } from './lynx';
import { alog } from './common/log';
import nativeGlobal from './common/nativeGlobal';
import { APP_SERVICE_NAME, DEFAULT_ENTRY, LynxFeature } from './common';
import { ReactApp } from './react/reactApp';
import { InternalRuntimeError, reportError } from './modules/report';
import StandaloneApp from './standalone/StandaloneApp';

export function loadCard(
  nativeApp: NativeApp,
  params: loadCardParams,
  lynx?: NativeLynxProxy
): boolean {
  const { id } = nativeApp;
  const { cardType } = params;
  alog(`load card native app id: ${id}`);
  let loadSuccess: boolean = true;
  let tt: ReactApp | StandaloneApp;
  try {
    if (cardType == 'standalone') {
      tt = new StandaloneApp({ nativeApp, params, lynx }, params);
    } else {
      tt = new ReactApp({
        nativeApp,
        params,
        lynx,
      });
    }
    nativeGlobal.currentAppId = id;
    nativeGlobal.multiApps[id] = tt;

    if (cardType === 'standalone') {
      nativeApp.setCard(tt);
      return true;
    }

    alog(
      `load card native app load app-service.js params.bundleSupportLoadScript ${params.bundleSupportLoadScript}`
    );
    loadSuccess = true;
    if (__WEB__) {
      tt.lynx.requireModuleAsync(APP_SERVICE_NAME, (error, ret) => {
        if (error) {
          tt.handleUserError(error);
        } else {
          if (tt.lynx._switches['allowUndefinedInNativeDataTypeSet']) {
            tt.dataTypeSet.add('undefined');
          }
        }
        nativeApp.setCard(tt);
      });
    }
    if (!__WEB__) {
      try {
        delete tt.lynx.requireModule.cache[APP_SERVICE_NAME];
        delete BaseApp._$factoryCache[APP_SERVICE_NAME];
        tt.lynx.requireModule(APP_SERVICE_NAME, DEFAULT_ENTRY);
        if (tt.lynx._switches['allowUndefinedInNativeDataTypeSet']) {
          tt.dataTypeSet.add('undefined');
        }
      } catch (e) {
        loadSuccess = false;
        tt.handleUserError(e, undefined, undefined, 'loadCard failed');
      }
    }
    nativeApp.setCard(tt);
  } catch (e) {
    handleLoadCardError(nativeApp, e);
    loadSuccess = false;
  }
  return loadSuccess;
}

export function destroyCard(id: string): void {
  alog(`destroy ${id}`);
  const appInstance = nativeGlobal.multiApps[id];
  appInstance.destroy();
  // eslint-disable-next-line @typescript-eslint/no-dynamic-delete
  delete nativeGlobal.multiApps[id];
}

export function callDestroyLifetimeFun(id: string): void {
  alog(`callDestroyLifetimeFun ${id}`);
  const appInstance = nativeGlobal.multiApps[id];
  appInstance.callDestroyLifetimeFun();
}

export function loadDynamicComponent<T>(tt: BaseApp, componentUrl: string): T {
  if (tt.loadedDynamicComponentsSet.has(componentUrl)) {
    return tt.getDynamicComponentExports(componentUrl);
  }

  const preEntry = nativeGlobal.globDynamicComponentEntry;
  nativeGlobal.globDynamicComponentEntry = componentUrl;

  try {
    delete tt.lynx.requireModule.cache[APP_SERVICE_NAME];
    delete BaseApp._$factoryCache[APP_SERVICE_NAME];
    const ret = tt.lynx.requireModule<T>(APP_SERVICE_NAME, componentUrl);
    tt.saveDynamicComponentExports(componentUrl, ret);
    tt.loadedDynamicComponentsSet.add(componentUrl);
    return ret;
  } catch (error) {
    tt.handleUserError(error);
  } finally {
    // Here reset globDynamicComponentEntry to avoid affect other LynxView in the same LynxGroup
    // detail see: #8720
    nativeGlobal.globDynamicComponentEntry = preEntry;
  }
}

export function handleLoadCardError(
  nativeApp: NativeApp,
  error?: Error,
  cause?: unknown
) {
  let { message, name, stack } = error || {};
  if (!message) {
    // If there is no error message in error, means that it is not an error-like object.
    // We construct a new Error using JSON.stringify
    ({ message, name, stack } = new Error(JSON.stringify(error)));
  }
  const internalError = new InternalRuntimeError(
    `loadCard failed ${name}: ${message}`,
    stack
  );
  internalError.cause = cause;
  reportError(internalError, nativeApp, {
    originError: error,
    getSourceMapRelease: (url: string): string => {
      let ret = nativeApp.__GetSourceMapRelease(url);
      if (!ret) {
        return nativeApp.__GetSourceMapRelease(BaseApp.kDefaultSourceMapURL);
      }
    },
  });
}
