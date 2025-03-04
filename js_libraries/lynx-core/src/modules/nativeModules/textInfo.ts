// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface TextInfo {
  fontSize: string;
  fontFamily?: string;
}

export interface TextMetrics {
  width: number;
}

export class TextInfoManager {
  private readonly _nativeModules: any;
  private _textInfoModule: any = undefined;

  constructor(nativeModules: object) {
    this._nativeModules = nativeModules;
  }

  getTextInfo = (param: any, options?: TextInfo): TextMetrics => {
    if (this._textInfoModule === undefined) {
      this._textInfoModule = this._nativeModules.LynxTextInfoModule;
    }
    if (this._textInfoModule && this._textInfoModule.getTextInfo) {
      return this._textInfoModule.getTextInfo(param, options);
    } else {
      return {
        width: param.length,
      };
    }
  };
}
