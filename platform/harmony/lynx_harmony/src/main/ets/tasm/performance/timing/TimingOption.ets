// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
import { TimingConstants } from './TimingConstants';


export class TimingOption {
  private pipelineOrigin: string = "";
  private timingInfo: Record<string, number> = {};

  constructor(pipelineOrigin: string) {
    this.pipelineOrigin = pipelineOrigin;
  }

  public setTiming(timingKey: string, msTimingStamp: number) {
    // convert ms to us
    this.timingInfo[timingKey] = msTimingStamp * 1000;
  }

  public toRecord(): Record<string, string | object> {
    let result: Record<string, string | object> = {};
    result[TimingConstants.PIPELINE_ORIGIN] = this.pipelineOrigin;
    result[TimingConstants.TIMESTAMP_MAP] = this.timingInfo
    return result;
  }
}