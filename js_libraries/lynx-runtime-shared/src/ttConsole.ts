// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { isFunction } from './utils';

export type SharedConsole = typeof nativeConsole & { runtimeId: string };

// TODO(wangqingyu): when using isolated context, console in local context
// should be created by createSharedConsole instead of using exported console
// the export default console below should only be used in the same context with lynx-kernel

function logWithRuntimeId(
  this: SharedConsole,
  funcName: string,
  ...args: unknown[]
) {
  // JS Vm use runtimeId:1 to recognize lynx debugging
  return nativeConsole[funcName].call(nativeConsole, this.runtimeId, ...args);
}

/**
 * Create a console that wrapped the nativeConsole to log with runtimeId.
 * @param runtimeId The runtimeId to be logged
 *
 * The runtimeId can be changed by setting directly.
 *
 * @example
 * const sharedConsole = createSharedConsole(runtimeId);
 * sharedConsole.runtimeId = anotherRuntimeId;
 */
export function createSharedConsole(runtimeId: string): SharedConsole {
  if (__OPEN_INTERNAL_LOG__ && NODE_ENV === 'development') {
    const sharedConsole = {} as SharedConsole;
    Object.keys(nativeConsole).forEach((funcName) => {
      // Should filter out those do not print out messages though CDP.
      // Using a forbidden-list here to filter out the API that does not print anything.
      // Not using hard-coded allow-list since it it more common for console APIs to print something.
      // So when we add new API like `console.table`, we don't need to modify here to get runtimeId injected.
      if (['profile', 'profileEnd'].includes(funcName)) {
        sharedConsole[funcName] = nativeConsole[funcName];
        return;
      }

      // For those not in forbidden-list, log with runtimeId
      if (isFunction(nativeConsole[funcName])) {
        sharedConsole[funcName] = logWithRuntimeId.bind(
          sharedConsole,
          funcName
        );
      }
    });
    sharedConsole.runtimeId = runtimeId;

    return sharedConsole;
  }

  return nativeConsole as SharedConsole;
}

const _global = (function () {
  // eslint-disable-next-line no-eval
  return this || (0, eval)('this');
})();

/**
 * This is a wrapper to nativeConsole that log with groupId.
 *
 * The groupId defaults to '-1' and can be changed.
 */
const groupConsole = createSharedConsole(`groupId:${_global.groupId || '-1'}`);

/**
 * All console in lynx-kernel should use this console
 */
export default NODE_ENV === 'development'
  ? groupConsole
  : (nativeConsole as SharedConsole);
