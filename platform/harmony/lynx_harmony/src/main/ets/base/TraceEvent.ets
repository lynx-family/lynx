// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { traceEventBegin, traceEventEnd, traceInstant } from 'liblynx.so';
import { LynxEnv } from '../tasm/LynxEnv';

export enum TraceCategory {
  Lynx = 'lynx',
  Vitals = 'vitals',
  Other = 'other'
}

export class TraceEvent {
  /**
   * Trace Event Start. Usage: TraceEvent.beginSection(TraceCategory.lynx, "LoadTemplate")
   * @param traceCategory
   * @param name Trace event name
   */
  static beginSection(traceCategory: TraceCategory, name: string, args?: Record<string, string>) {
    if (LynxEnv.isDebugModeEnabled()) {
      traceEventBegin(traceCategory, name, args);
    }
  }

  /**
   * Trace Event End. Usage: TraceEvent.endSection(TraceCategory.lynx, "LoadTemplate")
   * @param traceCategory
   */
  static endSection(traceCategory: TraceCategory, name: string) {
    if (LynxEnv.isDebugModeEnabled()) {
      traceEventEnd(traceCategory, name)
    }
  }

  /**
   * Trace Instant. Usage: TraceEvent.instant(TraceCategory.lynx, "draw_end")
   * @param traceCategory
   * @param name Trace instant name
   */
  static instant(traceCategory: TraceCategory, name: string, args?: Record<string, string>) {
    if (LynxEnv.isDebugModeEnabled()) {
      traceInstant(traceCategory, name, args);
    }
  }

  static enableTrace(): boolean {
    return LynxEnv.isDebugModeEnabled();
  }
}
