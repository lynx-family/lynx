// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import Url from '@ohos.url';
import { EmbeddedMode } from '@lynx/lynx';

export class LynxRecorderReplayOptions {
  embeddedMode: EmbeddedMode = EmbeddedMode.UNSET;
  private enableBTSOverride?: boolean;
  private recordedEnableJSRuntime?: boolean;

  constructor(url?: string) {
    if (!url) {
      return;
    }
    try {
      let urlObject = Url.URL.parseURL(url);
      let embeddedMode = urlObject.params.get('embedded_mode');
      if (embeddedMode !== null && embeddedMode !== undefined && /^-?\d+$/.test(embeddedMode)) {
        this.embeddedMode = parseInt(embeddedMode) as EmbeddedMode;
      }
      let enableBTS = urlObject.params.get('enable_bts');
      if (enableBTS !== null && enableBTS !== undefined) {
        this.enableBTSOverride = enableBTS !== '0';
      }
    } catch (error) {
      this.embeddedMode = EmbeddedMode.UNSET;
      this.enableBTSOverride = undefined;
    }
  }

  applyRecordedEnableJSRuntime(enableJSRuntime?: boolean): void {
    this.recordedEnableJSRuntime = enableJSRuntime;
  }

  get enableAirStrictMode(): boolean {
    return !(this.enableBTSOverride ?? this.recordedEnableJSRuntime ?? true);
  }
}
