// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import nativeConsole from './ttConsole';
let isNativeConsoleHasALog: boolean | undefined;

export function alog(str: string) {
  if (!__OPEN_INTERNAL_LOG__) {
    return;
  }
  if (isNativeConsoleHasALog === undefined) {
    isNativeConsoleHasALog = typeof nativeConsole.alog === 'function';
  }
  if (isNativeConsoleHasALog) {
    nativeConsole.alog('[LynxJSSDK]' + str);
  }
}

let isNativeConsoleHasReport: boolean | undefined;

export function report(str: string) {
  if (!__OPEN_INTERNAL_LOG__) {
    return;
  }
  if (isNativeConsoleHasReport === undefined) {
    isNativeConsoleHasReport = typeof nativeConsole.report === 'function';
  }
  if (isNativeConsoleHasReport) {
    nativeConsole.report('[LynxJSSDK]' + str);
  }
}
