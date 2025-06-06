// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.performance.timing;

public interface ITimingCollector {
  public void setMsTiming(final String key, final long msTimestamp, final String pipelineID);
  public void markTiming(final String key, final String pipelineID);
  public void setNeedPaintEndTiming(String pipelineId);
  public void markPaintEndTimingIfNeeded();
}
