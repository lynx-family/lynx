// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.base.trace;

import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.base.TraceEvent;
import java.util.HashMap;
import java.util.Map;

/**
 * Recording-gated sections shared by managed handles, transferred UIs, and shadow layout stages.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class TransferTrace {
  private TransferTrace() {}

  public static void begin(String stage, int instanceId, @Nullable String transferId, int nodeId) {
    if (!TraceEvent.enableTrace() || !TraceEvent.isTracingStarted()) {
      return;
    }
    Map<String, String> args = new HashMap<>();
    args.put("instance_id", String.valueOf(instanceId));
    if (transferId != null) {
      args.put("transferId", transferId);
    }
    if (nodeId >= 0) {
      args.put("nodeId", String.valueOf(nodeId));
    }
    TraceEvent.beginSection(stage, args);
  }

  public static void end(String stage) {
    if (!TraceEvent.enableTrace() || !TraceEvent.isTracingStarted()) {
      return;
    }
    TraceEvent.endSection(stage);
  }
}
