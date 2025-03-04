// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxClearTimeout, LynxSetTimeout } from '@lynx-js/types';
import { NativeModule } from '../modules';
import { Lynx } from '../lynx';

/**
 * An AMD like module with Lynx custom properties
 * @see https://github.com/amdjs/amdjs-api/wiki/AMD
 */
export interface AMDModule {
  /**
   * Value exports by this module
   */
  exports?: unknown;

  /**
   * The factory registered by `define`
   * Should be compatible with `app-service.js`
   */
  factory: AMDFactory;

  /**
   * Mark if this module is loaded and evaluated
   */
  hasRun: boolean;
}

export type AMDFactory = (
  ...args: [
    (path: string) => unknown, // require
    Partial<AMDModule>, // module
    unknown, // exports
    () => void, // Card
    LynxSetTimeout, // setTimeout
    LynxSetTimeout, // setInterval
    LynxClearTimeout, // clearInterval
    LynxClearTimeout, // clearTimeout
    NativeModule, // NativeModules
    Record<string, unknown>, // ApiList(tt)
    typeof console, // console
    () => void, // Component
    unknown, // ReactLynx
    string, // nativeAppId
    (() => void) | undefined, // Behavior
    unknown, // LynxJSBI
    Lynx, // lynx
    undefined, //  window
    undefined, //  document
    undefined, //  frames
    undefined, //  self
    undefined, //  location
    undefined, //  navigator
    undefined, //  localStorage
    undefined, //  history
    undefined, //  Caches
    undefined, //  screen
    undefined, //  alert
    undefined, //  confirm
    undefined, //  prompt
    typeof fetch, //  fetch
    undefined, //  XMLHttpRequest
    undefined, //  WebSocket
    undefined, //  webkit
    undefined, //  Reporter
    undefined, //  print
    undefined, //  global
    (cb?: (timeStamp?: number) => void) => number, // requestAnimationFrame
    (requestID?: number) => void // cancelAnimationFrame
  ]
) => void;
