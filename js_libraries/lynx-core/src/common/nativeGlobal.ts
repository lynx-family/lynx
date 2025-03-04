// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { DEFAULT_ENTRY } from './constants';
import { ShareDataSubject } from '../modules/sharedData/ShareDataSubject';
import { nativeGlobal as _global } from '@lynx-js/runtime-shared';

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
export const { loadScript } = _global;
export default _global;
