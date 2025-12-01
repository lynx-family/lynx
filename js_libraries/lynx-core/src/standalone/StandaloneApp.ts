// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { DEFAULT_ENTRY } from '../common';
import { AppProxyParams, BaseApp, loadCardParams, NativeApp } from '../app';
import { Lynx, NativeLynxProxy } from '../lynx';
import { CachedFunctionProxy } from '../util';

export default class StandaloneApp extends BaseApp {
  constructor(options: AppProxyParams<NativeApp>, params: loadCardParams) {
    super(options, undefined);
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
}
