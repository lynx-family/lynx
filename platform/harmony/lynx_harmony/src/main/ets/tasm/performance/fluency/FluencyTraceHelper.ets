// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxFpsTracer, LynxTracerContext } from './LynxFpsTracer';
import { LynxFpsRecord } from './LynxFpsRecord';
import { LynxEventReporter } from '../../eventreport/LynxEventReporter';
import { LynxReportEventPropValue } from '../../eventreport/ILynxEventReportObserver';

export class FluencyTraceHelper {
  private tracer: LynxFpsTracer = new LynxFpsTracer();

  constructor() {
  }

  destroy() {
    this.tracer.destroy();
  }

  start(sign: number, scene: string, tag: string, instanceId: number, probability: number) {
    if (!this.tracer) {
      return;
    }

    let context = new LynxTracerContext();
    context.scene = scene;
    context.tag = tag;
    context.pageConfigProbability = probability;
    context.instanceId = instanceId;
    let record = this.tracer.start(sign, context);
    // If one scroll event does not stop after 10 seconds, we stop it manually.
    let timeout = 10;
    record.setTimeout(timeout, (sign) => {
      this.stop(sign);
    })
  }

  stop(sign: number) {
    if (!this.tracer) {
      return;
    }

    let record = this.tracer.stop(sign);
    // just ignore scroll event with duration less than 200ms.
    if (record && record.rawMetrics.duration >= 20000) {
      this.doReport(record);
    }
  }

  private doReport(record: LynxFpsRecord) {
    let rawMetrics = record.rawMetrics;
    let metrics = record.getDerivedFpsMetrics();

    let builder = new Map<string, LynxReportEventPropValue>();
    builder.set('lynxsdk_fluency_scene', record.context.scene);
    builder.set('lynxsdk_fluency_tag', record.context.tag);
    builder.set('lynxsdk_fluency_maximum_frames', rawMetrics.maximumRefreshRate);

    // basic fluency info
    builder.set('lynxsdk_fluency_frames_number', rawMetrics.frames);
    builder.set('lynxsdk_fluency_dur', rawMetrics.duration);
    builder.set('lynxsdk_fluency_fps', metrics.fps);
    builder.set('lynxsdk_fluency_drop1_count', rawMetrics.drop1);
    builder.set('lynxsdk_fluency_drop1_duration', rawMetrics.drop1Duration);
    builder.set('lynxsdk_fluency_drop3_count', rawMetrics.drop3);
    builder.set('lynxsdk_fluency_drop3_duration', rawMetrics.drop3Duration);
    builder.set('lynxsdk_fluency_drop7_count', rawMetrics.drop7);
    builder.set('lynxsdk_fluency_drop7_duration', rawMetrics.drop7Duration);
    builder.set('lynxsdk_fluency_drop25_count', rawMetrics.drop25);
    builder.set('lynxsdk_fluency_drop25_duration', rawMetrics.drop25Duration);


    // frame loss ratio
    builder.set('lynxsdk_fluency_drop1_count_per_second',
      1000.0 * rawMetrics.drop1 / rawMetrics.duration);
    builder.set('lynxsdk_fluency_drop3_count_per_second',
      1000.0 * rawMetrics.drop3 / rawMetrics.duration);
    builder.set('lynxsdk_fluency_drop7_count_per_second',
      1000.0 * rawMetrics.drop7 / rawMetrics.duration);
    builder.set('lynxsdk_fluency_drop25_count_per_second',
      1000.0 * rawMetrics.drop25 / rawMetrics.duration);

    // frame loss time ratio
    builder.set(
      'lynxsdk_fluency_drop1_ratio', 1000.0 * rawMetrics.drop1Duration / rawMetrics.duration);
    builder.set(
      'lynxsdk_fluency_drop3_ratio', 1000.0 * rawMetrics.drop3Duration / rawMetrics.duration);
    builder.set(
      'lynxsdk_fluency_drop7_ratio', 1000.0 * rawMetrics.drop7Duration / rawMetrics.duration);
    builder.set('lynxsdk_fluency_drop25_ratio',
      1000.0 * rawMetrics.drop25Duration / rawMetrics.duration);

    builder.set('lynxsdk_fluency_pageconfig_probability',
      record.context.pageConfigProbability);

    LynxEventReporter.onEvent('lynxsdk_fluency_event', record.context.instanceId, builder);
  }
}