// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxResourceLoadInfo } from "../LynxViewClient";

export interface LynxImageLoadInfo extends LynxResourceLoadInfo {

  /**
   * The src of the image.
   */
  src: string;
  
  /**
   * Image width, Unit (vp)
   */
  width: number,

  /**
   * Image height, Unit (vp)
   */
  height: number,

  /**
   * Image view width, Unit (vp)
   */
  viewWidth?: number,

  /**
   * Image view height, Unit (vp)
   */
  viewHeight?: number,

  /**
   * Image start loading timestamp, Unit (ms).
   */
  loadStart?: number,

  /**
   * Image loading completion timestamp, Unit (ms)
   */
  loadFinish?: number,

  /**
   * Image loading duration (load_finish - load_start), Unit (ms).
   */
  cost?: number,

  /**
   * image resource from
   */
  origin?: number,
}
