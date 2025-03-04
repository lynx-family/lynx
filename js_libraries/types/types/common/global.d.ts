// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { SystemInfo } from './system-info';
import { Lynx as BackgroundLynx, NativeModules as INativeModules, GetElementByIdFunc } from '../background-thread';
import { Lynx as MainThreadLynx } from '../main-thread';
import { CommonLynx } from './lynx';

export type LynxSetTimeout = (callback: (...args: unknown[]) => unknown, number: number) => number;

export type LynxClearTimeout = (timeoutId: number) => void;

export type AnyObject = Record<string, any>;

export interface GlobalProps {}

export type UnsafeLynx = BackgroundLynx & MainThreadLynx;
export type SafeLynx = CommonLynx;

declare global {
  var SystemInfo: SystemInfo;
  var lynx: UnsafeLynx;

  var getElementById: GetElementByIdFunc;

  var NativeModules: INativeModules;

  /**
   * @description requestAnimationFrame
   * @since 3.0
   * below Lynx 3.0, use lynx.requestAnimationFrame.
   */
  function requestAnimationFrame(cb?: (timeStamp?: number) => void): number;

  /**
   * @description requestAnimationFrame
   * @since 3.0
   * below Lynx 3.0, use lynx.cancelAnimationFrame.
   */
  function cancelAnimationFrame(requestID?: number): void;
}

declare function setTimeout(callback: (...args: unknown[]) => unknown, number: number): number;
declare function setInterval(callback: (...args: unknown[]) => unknown, number: number): number;
declare function clearTimeout(timeoutId: number): void;
declare function clearInterval(timeoutId: number): void;
