// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.


import { FrameCallback } from '@kit.ArkUI';
import { TraceCategory, TraceEvent } from '../../base/TraceEvent';
import { TraceEventDef } from '../../base/TraceEventDef';
import { PerformanceController } from './PerformanceController';

export class DrawEndTimingFrameCallback extends FrameCallback {
  private perfController: PerformanceController;

  constructor(perfController: PerformanceController) {
    super();
    this.perfController = perfController;
  }

  onFrame(frameTimeNanos: number) {
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.DRAW_END_TIMING_ON_FRAME_CALLBACK);
    this.perfController.markPaintEndTimingIfNeeded();
    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.DRAW_END_TIMING_ON_FRAME_CALLBACK);
  }
}
