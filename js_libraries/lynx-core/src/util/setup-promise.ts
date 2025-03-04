// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxClearTimeout, LynxSetTimeout } from '@lynx-js/types';
import nativeGlobal from '../common/nativeGlobal';

type nextTick = (callback: () => void) => void;
export function getPromiseMaybePolyfill(
  setTimeout: LynxSetTimeout,
  onUnhandled,
  clearTimeout: LynxClearTimeout,
  queueMicrotask: nextTick = undefined,
  enableMicrotaskPromisePolyfill: boolean = false
) {
  const { getPromise } = nativeGlobal;
  if (typeof getPromise === 'function') {
    const nextTick = enableMicrotaskPromisePolyfill
      ? queueMicrotask
      : (fn: () => void) => setTimeout(fn, 0);
    return getPromise({ nextTick, setTimeout, onUnhandled, clearTimeout });
  } else {
    // TODO: should report error;
    return nativeGlobal.Promise;
  }
}
