// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance;

import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.performance.timing.TimingConstants;
import java.util.HashMap;

public class TimingOption {
  public final String pipelineOrigin;
  public final HashMap<String, Long> timingInfo;

  public TimingOption(String pipelineOrigin) {
    this.pipelineOrigin = pipelineOrigin;
    this.timingInfo = new HashMap<>();
  }

  public void setTiming(String key, long msTimingStamp) {
    // convert ms to us
    this.timingInfo.put(key, msTimingStamp * 1000);
  }

  public JavaOnlyMap toJavaOnlyMap() {
    JavaOnlyMap result = new JavaOnlyMap();
    result.putString(TimingConstants.PIPELINE_ORIGIN, this.pipelineOrigin);
    result.putMap(TimingConstants.TIMESTAMP_MAP, JavaOnlyMap.from(this.timingInfo));
    return result;
  }
}
