// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LLog } from '../../../base/LLog';
import { LynxTracerContext } from './LynxFpsTracer';

// Represents raw metrics for frames per second (FPS) analysis.
//
// Properties:
//   frames: number - The total number of frames rendered (count).
//   maximumRefreshRate: number - The maximum refresh rate supported (fps).
//   duration: number - The total duration of the recording in milliseconds (ms * 100).
//   hitchDuration: number - The total duration of hitches in milliseconds (ms * 100).
//   dropN: number - The count of N-frame drops (count).
//   dropNDuration: number - The total duration of N-frame drops in milliseconds (ms * 100).
export class LynxFpsRawMetrics {
  public frames: number = 0;
  public maximumRefreshRate: number = -1;
  public duration: number = 0;
  public hitchDuration: number = 0;
  public drop1: number = 0;
  public drop3: number = 0;
  public drop7: number = 0;
  public drop25: number = 0;
  public drop1Duration: number = 0;
  public drop3Duration: number = 0;
  public drop7Duration: number = 0;
  public drop25Duration: number = 0;
}

// Represents derived metrics for frames per second (FPS) analysis.
//
// Properties:
//   fps: number - The calculated frames per second (count/s).
//   dropNPerSecond: number - The number of N-frame drops per second (count/s).
//   hitchRatio: number - The ratio of hitch duration to total duration (s/s).
//   dropNRatio: number - The ratio of N-frame drop duration to total duration (ms/s).
export class LynxFpsDerivedMetrics {
  public fps: number = 0;
  public drop1PerSecond: number = 0;
  public drop3PerSecond: number = 0;
  public drop7PerSecond: number = 0;
  public drop25PerSecond: number = 0;
  public hitchRatio: number = 0;
  public drop1Ratio: number = 0;
  public drop3Ratio: number = 0;
  public drop7Ratio: number = 0;
  public drop25Ratio: number = 0;
}

export class LynxFpsRawFrame {
  public drop: number = 0;
  public duration: number = 0;
  public hitchDuration: number = 0;
}

export class LynxFpsRecord {
  private TAG: string = 'LynxFpsRecord';
  public key: number;
  public isStarted: boolean = false;
  public rawMetrics: LynxFpsRawMetrics = new LynxFpsRawMetrics();
  public context: LynxTracerContext;
  public TIMEOUT_INTERVAL: number = -1;
  public completion: (key: number) => void = (key: number) => {
    LLog.w(this.TAG, 'Default completion. Please call setTimeout before using completion.')
    return;
  }
  private OFFSET_TO_MS: number = 100;
  private TIME_S_TO_MS: number = 1000;

  constructor(key: number, context: LynxTracerContext) {
    this.key = key;
    this.context = context;
  }

  // Retrieves derived fluency metrics based on raw metrics data in record.
  //
  // Returns:
  //   An instance of LynxFpsDerivedMetrics containing calculated metrics,
  //   or undefined if the duration in metrics is zero.
  public getDerivedFpsMetrics(): LynxFpsDerivedMetrics {
    // convert unit from ms * 100 to s * 100
    let duration: number = this.rawMetrics.duration / this.TIME_S_TO_MS;
    let hitchDuration: number = this.rawMetrics.hitchDuration / this.TIME_S_TO_MS;
    let ret = new LynxFpsDerivedMetrics();
    ret.fps = this.rawMetrics.frames / duration * this.OFFSET_TO_MS;
    ret.drop1PerSecond = this.rawMetrics.drop1 / duration * this.OFFSET_TO_MS;
    ret.drop3PerSecond = this.rawMetrics.drop3 / duration * this.OFFSET_TO_MS;
    ret.drop7PerSecond = this.rawMetrics.drop7 / duration * this.OFFSET_TO_MS;
    ret.drop25PerSecond = this.rawMetrics.drop25 / duration * this.OFFSET_TO_MS;
    ret.hitchRatio = hitchDuration / duration;
    ret.drop1Ratio = this.rawMetrics.drop1Duration / duration;
    ret.drop3Ratio = this.rawMetrics.drop3Duration / duration;
    ret.drop7Ratio = this.rawMetrics.drop7Duration / duration;
    ret.drop25Ratio = this.rawMetrics.drop25Duration / duration;

    return ret;
  }

  // Set a completion callback to run on the record has been timed out.
  // Params:
  //   timeout – The timeout duration in seconds.
  //   completion – The function to be executed upon timeout, which receives a single parameter key (type: number),
  //                representing the unique identifier of the record.
  public setTimeout(timeout: number, completion: (key: number) => void) {
    if (timeout <= 0) {
      LLog.e(this.TAG, 'The timeout duration should be greater than 0.')
      return;
    }

    if (timeout > 0 && !completion) {
      LLog.e(this.TAG, 'Completion handler is required.')
      return;
    }

    this.TIMEOUT_INTERVAL = timeout * this.TIME_S_TO_MS * this.OFFSET_TO_MS;
    this.completion = completion;
  }
}