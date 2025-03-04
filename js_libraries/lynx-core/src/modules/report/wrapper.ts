// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import nativeConsole from '../../common/ttConsole';
import { isFunction, noop } from '@lynx-js/runtime-shared';
import { ErrorKind, UserRuntimeError, InternalRuntimeError } from './errors';
import { reportError } from './report-error';
import { RUN_TYPE, LYNX_CORE } from '../../common';
import { NativeApp } from '../../app';

type Instance = {
  _nativeApp: NativeApp;
  onError?: (error: string, errorObj: any) => void;
  __sourcemap__release__?: string;
  getSourceMapRelease?: (url: string) => string;
};

export function wrapUserFunction<T extends AnyFunction>(
  desc: string,
  instance: Instance,
  callback: T,
  runType: RUN_TYPE = LYNX_CORE
): T {
  if (!isFunction(callback)) return noop as T;
  return wrapFunction('USER_ERROR', desc, callback, instance, runType) as T;
}
function wrapFunction(
  errorKind: ErrorKind = 'INTERNAL_ERROR',
  desc: string,
  callback: AnyFunction,
  instance: Instance,
  runType: RUN_TYPE
) {
  return function wrapFunctionInner(...args) {
    try {
      return callback.apply(this, args);
    } catch (error) {
      const message = `${desc} \n${error.message}`;
      if (
        callback.name !== 'onError' &&
        typeof instance.onError === 'function'
      ) {
        instance.onError(
          `Card ${callback.name} exec error:${message}\n${error.stack}`,
          error
        );
      }
      const err =
        errorKind === 'INTERNAL_ERROR'
          ? new InternalRuntimeError(message, error.stack)
          : new UserRuntimeError(message, error.stack);
      nativeConsole.log(`wrapError-${desc}`, err);
      reportError(err, instance._nativeApp, {
        runType,
        __sourcemap__release__: instance.__sourcemap__release__,
        getSourceMapRelease: instance.getSourceMapRelease,
      });
    }
  };
}
export function wrapInnerFunction<T extends AnyFunction>(
  desc: string,
  instance: Instance,
  callback: T,
  runType: RUN_TYPE = LYNX_CORE
): T {
  if (!isFunction(callback)) return noop as T;
  return wrapFunction('INTERNAL_ERROR', desc, callback, instance, runType) as T;
}
