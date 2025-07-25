// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing;

import androidx.annotation.RestrictTo;

@RestrictTo(RestrictTo.Scope.LIBRARY)
public class TraceEventDef {
  public final static String CATEGORY = "disabled-by-default-devtools.fsp";
  public final static String FSP_TRACER_CREATE = "FSPTracer.create";
  public final static String FSP_TRACER_BEGIN_TRACING = "FSPTracer.beginTracing";
  public final static String FSP_TRACER_STOP_TRACING = "FSPTracer.stopTracing";
  public final static String FSP_TRACER_CAPTURE_SNAPSHOT = "FSPTracer.captureSnapshot";
  public final static String FSP_TRACER_UPDATE_SNAPSHOT = "FSPTracer.updateSnapshot";
  public final static String FSP_TRACER_IMPL_DIFF_SNAPSHOTS = "FSPTracerImpl.diffSnapshots";
  public final static String FSP_TRACER_IMPL_INSPECT_SNAPSHOT = "FSPTracerImpl.inspectSnapshot";
}
