// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.


import { display } from '@kit.ArkUI';

export class LynxScreenMetrics {
  //The unit is vp
  screenWidth: number;
  scale: number;
  screenPxWidth: number;
  screenPxHeight: number;

  constructor(screenWidth?: number) {
    let defaultDisplay = display.getDefaultDisplaySync();
    this.scale = defaultDisplay.densityPixels;
    this.screenPxWidth = defaultDisplay.width;
    this.screenPxHeight = defaultDisplay.height;
    this.screenWidth = this.screenPxWidth / this.scale;
    if (screenWidth) {
      this.screenWidth = screenWidth;
    }
  }
}
