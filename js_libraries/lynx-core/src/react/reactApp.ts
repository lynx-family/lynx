// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseApp } from '../app';
import { Lynx, NativeLynxProxy } from '../lynx';
import { CachedFunctionProxy } from '../util';

export class ReactApp extends BaseApp {
  createLynx(
    nativeLynx: NativeLynxProxy,
    promiseCtor: PromiseConstructor
  ): Lynx {
    const lynx_proxy = CachedFunctionProxy.create(nativeLynx);
    return new Lynx(
      () => this.nativeApp,
      () => this,
      promiseCtor,
      () => lynx_proxy
    );
  }

}
