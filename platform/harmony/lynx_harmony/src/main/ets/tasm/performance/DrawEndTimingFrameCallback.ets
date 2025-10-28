// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.


import { FrameCallback } from '@kit.ArkUI';
import { TraceCategory, TraceEvent } from '../../base/TraceEvent';
import { TraceEventDef } from '../../base/TraceEventDef';
import { PerformanceController } from './PerformanceController';

export class DrawEndTimingFrameCallback extends FrameCallback {
  private perfController: PerformanceController;
  private pipelineId: string;

  constructor(perfController: PerformanceController, pipelineId: string) {
    super();
    this.perfController = perfController;
    this.pipelineId = pipelineId;
  }

  onFrame(frameTimeNanos: number) {
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.DRAW_END_TIMING_ON_FRAME_CALLBACK);
    this.perfController.markTiming("paintEnd", this.pipelineId);
    // The rendering pipeline is considerd to be complete after the kPaintEnd phase,
    // so we mark pipelineEnd here
    this.perfController.markTiming("pipelineEnd", this.pipelineId);
    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.DRAW_END_TIMING_ON_FRAME_CALLBACK);
  }
}
