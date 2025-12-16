// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { TraceControllerHarmony } from 'liblynx.so';

export interface TraceConfig {
  filePath?: string | undefined,
  bufferSize?: number | undefined,
  enableCompress?: string | undefined,
}

export class TraceController {
  private static instance: TraceController = new TraceController();
  private traceSessionId: number = -1;

  private constructor() {
  }

  public static getInstance(): TraceController {
    return TraceController.instance;
  }

  /**
   * startTracing. usage: TraceController.getInstance().startTracing();
   * @param traceConfig
   */
  public startTracing(traceConfig?: TraceConfig) {
    //  Cannot start a new Trace while the previous Trace is not yet finished.
    if (this.traceSessionId != -1) {
      return;
    }
    this.traceSessionId = TraceControllerHarmony.startTracing(traceConfig);
  }

  /**
   * stopTracing. usage: TraceController.getInstance().stopTracing();
   */
  public stopTracing(): boolean {
    if (this.traceSessionId != -1) {
      const result = TraceControllerHarmony.stopTracing(this.traceSessionId);
      this.traceSessionId = -1;
      return result;
    } else {
      return false;
    }
  }
}