// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import display from '@ohos.display';

export class DisplayInfo {
  private _densityPixels: number;
  private _densityDPI: number;
  private _screenWidth: number;
  private _screenHeight: number;
  private _refreshRate: number;

  constructor(info?: DisplayInfo) {
    if (info) {
      this._densityPixels = info._densityPixels;
      this._densityDPI = info._densityDPI;
      this._screenWidth = info._screenWidth;
      this._screenHeight = info._screenHeight;
      this._refreshRate = info._refreshRate;
    } else {
      const defaultDisplay = display.getDefaultDisplaySync();
      this._densityPixels = defaultDisplay.densityPixels;
      this._densityDPI = defaultDisplay.densityDPI;
      this._screenWidth = defaultDisplay.width;
      this._screenHeight = defaultDisplay.height;
      this._refreshRate = defaultDisplay.refreshRate;
    }
  }

  updateDisplayInfo(width: number, height: number, densityPixels?: number, densityDPI?: number, refreshRate?: number) {
    this._screenWidth = width;
    this._screenHeight = height;
    if (densityPixels) {
      this._densityPixels = densityPixels;
    }
    if (densityDPI) {
      this._densityDPI = densityDPI;
    }
    if (refreshRate) {
      this._refreshRate = refreshRate;
    }
  }

  get densityPixels() {
    return this._densityPixels;
  }

  get densityDPI() {
    return this._densityDPI;
  }

  get screenWidth() {
    return this._screenWidth;
  }

  get screenHeight() {
    return this._screenHeight;
  }

  get refreshRate() {
    return this._refreshRate;
  }

  clone(): DisplayInfo {
    return new DisplayInfo(this);
  }
}
