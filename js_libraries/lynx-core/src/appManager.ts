// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// start js app, native has decode js code;
// return means loadCard success or failed.
import { BaseApp, loadCardParams, NativeApp } from './app';
import { Lynx, NativeLynxProxy } from './lynx';
import { alog } from './common/log';
import nativeGlobal, {
  PageGlobal,
  resolvePageGlobal,
  seedPageGlobal,
} from './common/nativeGlobal';
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
  seedPageGlobal(params);
  const pageGlobal = resolvePageGlobal(params);
  try {
    if (cardType == 'standalone') {
      tt = new StandaloneApp({ nativeApp, params, lynx }, params);
    } else {
      tt = new ReactApp(
        {
          nativeApp,
          params,
          lynx,
        },
        pageGlobal.multiApps[id]
      );
    }
    pageGlobal.currentAppId = id;
    pageGlobal.multiApps[id] = tt;

    if (cardType === 'standalone') {
      nativeApp.setCard(tt);
      return true;
    }

    tt.onAppReload = (
      updateData?: object,
      options?: { processorName?: string }
    ): void => {
      reloadCard(tt as BaseApp, updateData, options);
    };

    alog(
      `load card native app load app-service.js params.bundleSupportLoadScript ${params.bundleSupportLoadScript}`
    );
    loadSuccess = true;
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
    nativeApp.setCard(tt);
  } catch (e) {
    handleLoadCardError(nativeApp, e);
    loadSuccess = false;
  }
  return loadSuccess;
}

/**
 * Reload a card by evaluating its entry again, instead of re-rendering whatever
 * the framework kept from the previous render.
 *
 * {@link loadCard} installs this as the default `onAppReload`, so native's
 * `App::OnAppReload` lands here unless the framework overrides it. The old app
 * is torn down first, then {@link loadCard} builds a fresh app that reuses the
 * native plumbing (`nativeApp`, `lynx`, the event emitter) while app-service.js
 * and the bundles it pulls in are evaluated again, so their module scoped state
 * starts over.
 */
export function reloadCard(
  tt: BaseApp,
  updateData?: object,
  options?: { processorName?: string }
): boolean {
  alog(`reload card native app id: ${tt.nativeAppId}`);
  tt.callDestroyLifetimeFun?.();

  // The next app reuses this lynx, and with it the modules this one required.
  // `loadCard` only busts the app-service entry, which for a bundled app is a
  // stub that requires the real entry chunk, so drop the whole page's cache or
  // that chunk comes back from it instead of being evaluated again.
  const cache = tt.lynx.requireModule.cache;
  for (const path of Object.keys(cache)) {
    // eslint-disable-next-line @typescript-eslint/no-dynamic-delete
    delete cache[path];
  }

  return loadCard(
    tt.nativeApp,
    {
      ...tt.params,
      updateData,
      processorName: options?.processorName,
      isReload: true,
    },
    tt.lynx.getNativeLynx()
  );
}

export function destroyCard(
  id: string,
  pageGlobal: PageGlobal = nativeGlobal
): void {
  alog(`destroy ${id}`);
  const appInstance = pageGlobal.multiApps[id];
  appInstance.destroy();
  // The shared-data subject is group-wide and outlives this page. Any observer
  // this page left behind is a function object of the page's own realm, which
  // would keep that realm alive, so drop them all here.
  nativeGlobal.shareDataSubject.removeObserversOfOwner(id);
  // eslint-disable-next-line @typescript-eslint/no-dynamic-delete
  delete pageGlobal.multiApps[id];
}

export function callDestroyLifetimeFun(
  id: string,
  pageGlobal: PageGlobal = nativeGlobal
): void {
  alog(`callDestroyLifetimeFun ${id}`);
  const appInstance = pageGlobal.multiApps[id];
  appInstance.callDestroyLifetimeFun();
}

export function loadDynamicComponent<T>(tt: BaseApp, componentUrl: string): T {
  if (tt.loadedDynamicComponentsSet.has(componentUrl)) {
    return tt.getDynamicComponentExports(componentUrl);
  }

  // Scoped to the page being loaded, so it lives on that page's realm.
  const pageGlobal = tt.pageGlobal;
  const preEntry = pageGlobal.globDynamicComponentEntry;
  pageGlobal.globDynamicComponentEntry = componentUrl;

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
    pageGlobal.globDynamicComponentEntry = preEntry;
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
