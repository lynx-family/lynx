// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import EventEmitter from '../event';
import {
  Performance as IPerformance,
  TimingListener,
  PerformanceCallback,
} from '@lynx-js/types';
import { NativeApp } from '../../app';
import { TraceOption } from '@lynx-js/types/types/common/performance';
import { PerformanceObserver } from './performanceObserver';

const ListenerKeys = {
  onSetup: 'lynx.performance.timing.onSetup',
  onUpdate: 'lynx.performance.timing.onUpdate',
};

export interface PipelineOptions {
  pipelineID: string;
  needTimestamps: boolean;
}

export default class Performance implements IPerformance {
  _emitter: EventEmitter;
  _generatePipelineOptions: () => PipelineOptions;
  _onPipelineStart: (pipeline_id: string) => void;
  _markTiming: (pipeline_id: string, timing_key: string) => void;
  _profileStart: (traceName: string, option?: TraceOption) => void;
  _profileEnd: (option?: TraceOption) => void;
  _profileMark: (traceName: string, option?: TraceOption) => void;
  _profileFlowId: () => number;
  _isProfileRecording: () => boolean;
  _bindPipelineIdWithTimingFlag: (
    pipeline_id: string,
    timing_flag: string
  ) => void;
  constructor(emitter: EventEmitter, nativeApp: NativeApp) {
    this._emitter = emitter;
    this._generatePipelineOptions = nativeApp.generatePipelineOptions;
    this._onPipelineStart = nativeApp.onPipelineStart;
    this._markTiming = nativeApp.markPipelineTiming;
    this._profileStart = nativeApp.profileStart;
    this._profileEnd = nativeApp.profileEnd;
    this._profileMark = nativeApp.profileMark;
    this._profileFlowId = nativeApp.profileFlowId;
    this._isProfileRecording = nativeApp.isProfileRecording;
    this._bindPipelineIdWithTimingFlag = nativeApp.bindPipelineIdWithTimingFlag;
  }

  profileStart(traceName: string, option?: TraceOption) {
    this._profileStart(traceName, option);
  }

  profileEnd() {
    this._profileEnd();
  }

  profileMark(traceName: string, option?: TraceOption) {
    this._profileMark(traceName, option);
  }

  profileFlowId() {
    return this._profileFlowId();
  }

  createObserver(callback: PerformanceCallback): PerformanceObserver {
    return new PerformanceObserver(this._emitter, callback);
  }

  isProfileRecording() {
    return this._isProfileRecording();
  }

  addTimingListener(listener: TimingListener): void {
    this._emitter.addListener(ListenerKeys.onSetup, listener.onSetup, listener);
    this._emitter.addListener(
      ListenerKeys.onUpdate,
      listener.onUpdate,
      listener
    );
  }

  removeTimingListener(listener: TimingListener) {
    this._emitter.removeListener(ListenerKeys.onSetup, listener.onSetup);
    this._emitter.removeListener(ListenerKeys.onUpdate, listener.onUpdate);
  }

  removeAllTimingListener() {
    this._emitter.removeAllListeners(ListenerKeys.onSetup);
    this._emitter.removeAllListeners(ListenerKeys.onUpdate);
  }
  _initializeAndStartPipeline(): PipelineOptions {
    const pipelineOptions = this._generatePipelineOptions();
    if (pipelineOptions) {
      this._onPipelineStart(pipelineOptions.pipelineID);
    }
    return pipelineOptions;
  }
  _checkAndBindTimingFlag(
    pipelineOptions: PipelineOptions,
    data: Record<string, unknown>
  ) {
    if (!pipelineOptions) {
      return;
    }
    const PerformanceTimingFlag = '__lynx_timing_flag';
    if (data[PerformanceTimingFlag]) {
      this._bindPipelineIdWithTimingFlag(
        pipelineOptions.pipelineID,
        data[PerformanceTimingFlag] as string
      );
      this._markTiming(pipelineOptions.pipelineID, 'update_set_state_trigger');
      pipelineOptions.needTimestamps = true;
    }
  }
}
