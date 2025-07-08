// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import displaySync from '@ohos.graphics.displaySync';
import { HashMap } from '@kit.ArkTS';
import { LynxFpsRawFrame, LynxFpsRawMetrics, LynxFpsRecord } from './LynxFpsRecord';
import { LLog } from '../../../base/LLog';
import display from '@ohos.display';
import { LynxEventReporter } from '../../eventreport/LynxEventReporter';

const TAG: string = 'LynxFpsTracer'
const INVALID_TIMESTAMP: number = -1;

export class LynxFpsTracer {
  private lastTimestampNanos: number = INVALID_TIMESTAMP;
  private lastTargetTimestampNanos: number = INVALID_TIMESTAMP;
  private displaySync: displaySync.DisplaySync;
  private keyedRecords: HashMap<number, LynxFpsRecord> = new HashMap<number, LynxFpsRecord>();
  private timeoutRecords: Set<LynxFpsRecord> = new Set<LynxFpsRecord>();
  // To improve accuracy, the saved time unit is ms * 100.
  private OFFSET_TO_MS: number = 100;
  private TIME_MS_TO_NS: number = 1000000;
  private FACTOR: number = this.TIME_MS_TO_NS / this.OFFSET_TO_MS;
  private desiredFrameInterval: number = 1000 * this.OFFSET_TO_MS / 60; // 1666

  constructor() {
    this.displaySync = displaySync.create();
    this.displaySync.on('frame', this.frameCallback);
  }

  destroy() {
    this.displaySync.off('frame', this.frameCallback);
  }

  private frameCallback: Callback<displaySync.IntervalInfo> = (frameInfo: displaySync.IntervalInfo) => {
    let currentTimeStampNanos: number = frameInfo.timestamp;
    // get maximum refresh rate
    let defaultDisplay = display.getDefaultDisplaySync();
    let maximumRefreshRate = defaultDisplay.refreshRate;

    if (this.lastTimestampNanos !== INVALID_TIMESTAMP) {
      let frame = new LynxFpsRawFrame();
      frame.duration = Math.floor((currentTimeStampNanos - this.lastTimestampNanos) / this.FACTOR);
      frame.hitchDuration = Math.floor((currentTimeStampNanos - this.lastTargetTimestampNanos) / this.FACTOR);
      // Currently, the value of hitchDuration may be -1, which we consider to also indicate that
      // the frame started rendering at the expected time, so we handled it as 0.
      if (frame.hitchDuration < 0) {
        frame.hitchDuration = 0;
      } else {
        if (frame.hitchDuration >= 25.0 * this.desiredFrameInterval) {
          frame.drop = 25;
        } else if (frame.hitchDuration >= 7.0 * this.desiredFrameInterval) {
          frame.drop = 7;
        } else if (frame.hitchDuration >= 3.0 * this.desiredFrameInterval) {
          frame.drop = 3;
        } else if (frame.hitchDuration >= this.desiredFrameInterval) {
          frame.drop = 1;
        }
      }

      // Update all records
      for (const record of this.keyedRecords.values()) {
        if (record.rawMetrics.maximumRefreshRate == -1) {
          record.rawMetrics.maximumRefreshRate = maximumRefreshRate;
        }

        let oldDuration = record.rawMetrics.duration;
        this.LynxFPSRawMetricsAddFrame(record.rawMetrics, frame);

        let newDuration = record.rawMetrics.duration;
        let timeout = record.TIMEOUT_INTERVAL;
        if (timeout > 0 && oldDuration < timeout && newDuration >= timeout) {
          this.timeoutRecords.add(record);
        }
      }

      // Process and clear all records that have timed out.
      for (const record of this.timeoutRecords) {
        record.completion(record.key);
      }
      this.timeoutRecords.clear();
    }
    this.lastTimestampNanos = currentTimeStampNanos;
    this.lastTargetTimestampNanos = frameInfo.targetTimestamp;
  }

  private LynxFPSRawMetricsAddFrame(metrics: LynxFpsRawMetrics, frame: LynxFpsRawFrame) {
    metrics.frames += 1;
    metrics.duration += frame.duration;
    metrics.hitchDuration += frame.hitchDuration;

    // dropN means the frame drop interval is [N, +∞)
    switch (frame.drop) {
      case 25:
        metrics.drop25 += 1;
        metrics.drop25Duration += frame.hitchDuration;
      case 7:
        metrics.drop7 += 1;
        metrics.drop7Duration += frame.hitchDuration;
      case 3:
        metrics.drop3 += 1;
        metrics.drop3Duration += frame.hitchDuration;
      case 1:
        metrics.drop1 += 1;
        metrics.drop1Duration += frame.hitchDuration;
      default:
        break;
    }
  }

  start(key: number, context: LynxTracerContext): LynxFpsRecord {
    LLog.i(TAG, `LynxFpsTracer start record ${key}`);
    let record: LynxFpsRecord = this.keyedRecords.get(key);
    if (record) {
      this.startRecord(record);
      return record;
    }

    record = new LynxFpsRecord(key, context);
    this.keyedRecords.set(key, record)
    this.startRecord(record);
    return record;
  }

  private startRecord(record: LynxFpsRecord) {
    if (record.isStarted) {
      return;
    }
    record.isStarted = true;
    this.displaySync.start();
  }

  stop(key: number): LynxFpsRecord | undefined {
    LLog.i(TAG, `LynxFpsTracer stop record ${key}`);
    let record: LynxFpsRecord = this.keyedRecords.get(key);
    if (record) {
      this.stopRecord(record);
    }
    return record;
  }

  private stopRecord(record: LynxFpsRecord) {
    if (!record.isStarted) {
      return;
    }
    record.isStarted = false;
    this.keyedRecords.remove(record.key);

    if (this.keyedRecords.length == 0) {
      this.stopListenDisplaySync();
    }
  }

  private stopListenDisplaySync() {
    this.displaySync.stop();
    this.lastTimestampNanos = INVALID_TIMESTAMP;
    this.lastTargetTimestampNanos = INVALID_TIMESTAMP;
  }
}

export class LynxTracerContext {
  public scene: string = '';
  public tag: string = '';
  public instanceId: number = LynxEventReporter.INSTANCE_ID_UNKNOWN;
  public pageConfigProbability: number = LynxTracerContext.UNKNOWN_FLUENCY_PAGECONFIG_PROBABILITY;
  public static UNKNOWN_FLUENCY_PAGECONFIG_PROBABILITY: number = -1;
}