// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import nativeConsole from '../../common/ttConsole';
import { LYNX_CORE, RUN_TYPE } from '../../common';
import { App, NativeApp } from '../../app';
import { isObject } from '@lynx-js/runtime-shared';
import { BaseError, LynxErrorLevel } from './errors';

export function reportError(
  error: BaseError,
  nativeApp: NativeApp,
  options?: {
    runType?: RUN_TYPE;
    originError?: any;
    __sourcemap__release__?: string;
    getSourceMapRelease?: (url: string) => string;
    errorCode?: number;
    errorLevel?: LynxErrorLevel;
  }
): void {
  const { originError, errorCode, errorLevel, runType = LYNX_CORE } =
    options ?? {};
  nativeConsole.error('The following error occurred in the JSRuntime:');
  nativeConsole.error(`${error?.message}\n${error?.stack}`);
  error.cause = isObject(error.cause)
    ? JSON.stringify(error.cause)
    : error.cause;
  try {
    nativeApp.reportException(error, {
      ...runType,
      buildVersion: __BUILD_VERSION__,
      versionCode: __VERSION__,
      errorCode,
      errorLevel,
    });
  } catch (error) {
    nativeConsole.error('reportError err:\n', error);
  }
}

export function legacyReportError(
  error: BaseError,
  nativeApp: NativeApp,
  runType = LYNX_CORE,
  originError?: any,
  proxy?: App
) {
  return reportError(error, nativeApp, {
    runType,
    originError,
    __sourcemap__release__: proxy.__sourcemap__release__,
  });
}

export function reportThrowError({
  error,
  nativeApp,
  runType = LYNX_CORE,
  rawError,
  __sourcemap__release__,
  getSourceMapRelease,
}: {
  error: BaseError;
  nativeApp: NativeApp;
  runType?: RUN_TYPE;
  rawError: object;
  __sourcemap__release__?: string;
  getSourceMapRelease?: (url: string) => string;
}): void {
  reportError(error, nativeApp, {
    originError: rawError,
    runType,
    __sourcemap__release__,
    getSourceMapRelease,
  });
}
