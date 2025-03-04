// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { isError, isString } from '@lynx-js/runtime-shared';
import { BaseApp, NativeApp } from '../../app';
import { alog } from '../../common/log';

export class Reporter {
  constructor(
    private getApp: () => BaseApp,
    private readonly getNativeApp: () => NativeApp
  ) {
    this.getApp = getApp;
    this.getNativeApp = getNativeApp;
  }

  public rebind(getApp: () => BaseApp) {
    this.getApp = getApp;
  }

  // /**
  //  * key url -> value sourcemap
  //  * support different sourcemap for external js
  //  */
  // sourcemaps: Record<string, string> = {};

  /**
   * Set sourcemap release with a newly thrown error
   * @param {Error} error
   * The error thrown from the file that wants to set sourcemap release.
   * The top frame of `error.stack` **must be** the filename.
   * The `error.name` **must be** `'LynxGetSourceMapReleaseError'`.
   * The `error.message` **must be** the sourcemap release.
   *
   * @example
   * (function () {
   *   try {
   *     throw new Error(sourcemapRelease);
   *   } catch (e) {
   *     e.name = 'LynxGetSourceMapReleaseError';
   *     tt.setSourceMapRelease(e);
   *   }
   * })()
   */
  setSourceMapRelease = (error: Error) => {
    if (
      isError(error) &&
      error.name === BaseApp.kGetSourceMapReleaseErrorName &&
      isString(error.message) &&
      isString(error.stack)
    ) {
      this.getNativeApp().__SetSourceMapRelease({
        name: error.name,
        message: error.message,
        stack: error.stack,
      });
      return;
    }
    alog(`setSourceMapRelease failed with error: ${JSON.stringify(error)}`);
  };

  getSourceMapRelease = (url: string): string => {
    let ret = this.getNativeApp().__GetSourceMapRelease(url);
    if (!ret) {
      return this.getNativeApp().__GetSourceMapRelease(
        BaseApp.kDefaultSourceMapURL
      );
    }
  };
}
