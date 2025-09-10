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

  /**
   * Creates a TimingOption instance with initial timing measurements.
   * This factory method initializes a TimingOption with the current system time
   * for both pipeline start and the specified start timing type.
   *
   * @param pipelineOrigin The origin identifier for the pipeline
   * @param startTimingType The specific timing type to mark as started
   * @return A new TimingOption instance with initial timing data
   */
  public static TimingOption createTimingOption(String pipelineOrigin, String startTimingType) {
    TimingOption timingOption = new TimingOption(pipelineOrigin);
    long currentTimeMillis = PerformanceController.currentSystemTimeMicroseconds();
    timingOption.setTiming(TimingConstants.PIPELINE_START, currentTimeMillis);
    timingOption.setTiming(startTimingType, currentTimeMillis);
    return timingOption;
  }

  public TimingOption(String pipelineOrigin) {
    this.pipelineOrigin = pipelineOrigin;
    this.timingInfo = new HashMap<>();
  }

  public void setTiming(String key, long usTimingStamp) {
    this.timingInfo.put(key, usTimingStamp);
  }

  public void markTiming(String key) {
    long currentTimeMillis = PerformanceController.currentSystemTimeMicroseconds();
    this.timingInfo.put(key, currentTimeMillis);
  }

  public JavaOnlyMap toJavaOnlyMap() {
    JavaOnlyMap result = new JavaOnlyMap();
    result.putString(TimingConstants.PIPELINE_ORIGIN, this.pipelineOrigin);
    result.putMap(TimingConstants.TIMESTAMP_MAP, JavaOnlyMap.from(this.timingInfo));
    return result;
  }
}
