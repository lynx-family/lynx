// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
import {
  loadCard,
  destroyCard,
  callDestroyLifetimeFun,
  loadDynamicComponent,
} from './appManager';
import { arrayBufferToBase64, base64ToArrayBuffer } from './polyfill';
import nativeGlobal from './common/nativeGlobal';
import {
  createEventEmitter,
  legacyReportError,
  wrapInnerFunction,
  wrapUserFunction,
} from './modules';
import {
  Headers,
  URL,
  URLSearchParamsPolyfill,
  AbortController,
  AbortSignal,
} from './modules/fetch';

export { loadCard, destroyCard, callDestroyLifetimeFun, loadDynamicComponent };

nativeGlobal.loadCard = loadCard;
nativeGlobal.destroyCard = destroyCard;
nativeGlobal.callDestroyLifetimeFun = callDestroyLifetimeFun;
nativeGlobal.loadDynamicComponent = loadDynamicComponent;
/**
 * only for lynx native runtime
 */
nativeGlobal.__createEventEmitter = createEventEmitter;
nativeGlobal.__lynxArrayBufferToBase64 = arrayBufferToBase64;
nativeGlobal.__lynxBase64ToArrayBuffer = base64ToArrayBuffer;
nativeGlobal.LynxSDKCore = {
  report: legacyReportError,
  reportInner: wrapInnerFunction,
  reportUser: wrapUserFunction,
};

nativeGlobal.Headers = Headers;
nativeGlobal.AbortController = AbortController;
nativeGlobal.AbortSignal = AbortSignal;
nativeGlobal.URL = URL;
URLSearchParamsPolyfill(nativeGlobal);
