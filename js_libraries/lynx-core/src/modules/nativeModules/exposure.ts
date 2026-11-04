// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export class ExposureManager {
  private readonly _nativeModules: any;
  private _cachedExposureModule: any;

  constructor(nativeModules: object) {
    this._nativeModules = nativeModules;
  }

  private get _exposureModule(): any {
    // Page modules may become available after a standalone runtime is attached.
    return (this._cachedExposureModule ??= this._nativeModules.LynxExposureModule);
  }

  resumeExposure = (): void => {
    this._exposureModule.resumeExposure();
  };

  stopExposure = (options?: { sendEvent?: boolean }): void => {
    this._exposureModule.stopExposure(options);
  };

  setObserverFrameRate = (options?: {
    forPageRect?: number;
    forExposureCheck?: number;
  }): void => {
    this._exposureModule.setObserverFrameRate(options);
  };
}
