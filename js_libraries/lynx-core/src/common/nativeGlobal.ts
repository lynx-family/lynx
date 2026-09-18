// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { DEFAULT_ENTRY } from './constants';
import { ShareDataSubject } from '../modules/sharedData/ShareDataSubject';
import { nativeGlobal as _global } from '@lynx-js/runtime-shared';
import { LynxNapiLoader } from '@lynx-js/types';

// for card.
_global.multiApps = {};
_global.currentAppId = '';
_global.globComponentRegistPath = '';
_global.sharedData = {};
_global.globDynamicComponentEntry = DEFAULT_ENTRY;

_global.shareDataSubject = new ShareDataSubject();

_global.TaroLynx = {};
// bundle run with no eval
_global.bundleSupportLoadScript = true;
// for napi
_global.getNapiLoader = (): LynxNapiLoader | undefined => {
  return _global.__lynxNapiLoader;
};

export const { loadScript } = _global;
export default _global;

// globalThis of the realm a page's JS runs in. Same object as the default
// export on the legacy path; a separate realm once corejs runs on the group
// global context, where page-installed values are unreachable via nativeGlobal.
export type PageGlobal = typeof _global;

// Page realm injected by native as `currentGlobalThis`; absent on legacy path.
export function resolvePageGlobal(params?: {
  currentGlobalThis?: Record<string, any>;
}): PageGlobal {
  return (params?.currentGlobalThis as PageGlobal) ?? _global;
}

// Seed a page realm with the globals page bundles read off their own realm:
// `globComponentRegistPath` is bare-assigned under 'use strict' (throws without
// the slot), `globDynamicComponentEntry` needs its default, and bundles build
// their Promise via `getPromise`, a stateless factory safe to share.
export function seedPageGlobal(params?: {
  currentGlobalThis?: Record<string, any>;
}): void {
  const pageGlobal = params?.currentGlobalThis;
  if (!pageGlobal || pageGlobal === _global) {
    return;
  }
  pageGlobal.globComponentRegistPath = '';
  pageGlobal.globDynamicComponentEntry = DEFAULT_ENTRY;
  pageGlobal.getPromise = _global.getPromise;
}
