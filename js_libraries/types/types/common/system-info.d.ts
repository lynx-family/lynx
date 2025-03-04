// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export type PlatformType = 'Android' | 'iOS';

export interface SystemInfo {
  /**
   * The version of the Lynx SDK.
   * @example '2.4', '2.10'
   */
  readonly lynxSdkVersion: string;

  /**
   * The current operating system version.
   */
  readonly osVersion: string;

  /**
   * The physical pixel height of the real device.
   */
  readonly pixelHeight: number;

  /**
   * The physical pixel width of the real device.
   */
  readonly pixelWidth: number;

  /**
   * The physical pixel ratio of the real device.
   */
  readonly pixelRatio: number;

  /**
   * The platform of the current device.
   */
  readonly platform: PlatformType;

  /**
   * The JavaScript engine currently used.
   * @note Not available in lepus
   */
  readonly runtimeType: 'v8' | 'jsc' | 'quickjs';

  readonly theme?: object;
}
