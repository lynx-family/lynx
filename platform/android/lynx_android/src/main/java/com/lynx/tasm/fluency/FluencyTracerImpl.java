// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.fluency;

import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.eventreport.LynxEventReporter;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

class FluencyTracerImpl {
  private final WeakReference<LynxContext> mContext;
  private static final String LYNXSDK_FLUENCY_EVENT = "lynxsdk_fluency_event";
  private Map<Integer, LynxFpsTracer> mKeyedTracer = new HashMap<>();

  FluencyTracerImpl(LynxContext context) {
    mContext = new WeakReference<>(context);
  }

  public static class FluencyTracerConfig {
    public String scene = "";
    public String tag = "";
    public double pageConfigProbability = FluencyTraceHelper.UNKNOWN_FLUENCY_PAGECONFIG_PROBABILITY;
  }

  public void start(int sign, FluencyTracerConfig config) {
    LynxFpsTracer tracer = mKeyedTracer.get(sign);
    if (tracer == null) {
      LynxContext lynxContext = mContext.get();
      if (lynxContext == null) {
        return;
      }
      tracer = initLynxTracer(lynxContext, config);
      mKeyedTracer.put(sign, tracer);
    }
    tracer.start();
  }

  public void stop(int sign) {
    LynxFpsTracer tracer = mKeyedTracer.get(sign);
    if (tracer != null) {
      tracer.stop();
      mKeyedTracer.remove(sign);
    }
  }

  private LynxFpsTracer initLynxTracer(LynxContext context, FluencyTracerConfig config) {
    LynxFpsTracer fpsTracer = new LynxFpsTracer(context);
    fpsTracer.setFluencyCallback(new LynxFluencyCallback(config, context.getInstanceId()));
    return fpsTracer;
  }

  private static class LynxFluencyCallback implements LynxFpsTracer.IFluencyCallback {
    private final FluencyTracerConfig mConfig;
    private final int mInstanceId;

    public LynxFluencyCallback(FluencyTracerConfig config, int instanceId) {
      mConfig = config;
      mInstanceId = instanceId;
    }

    @Override
    public void report(LynxFpsTracer.LynxFpsRawMetrics rawMetrics) {
      LynxEventReporter.PropsBuilder builder = () -> {
        Map<String, Object> props = new HashMap<>();
        props.put("lynxsdk_fluency_scene", mConfig.scene);
        props.put("lynxsdk_fluency_tag", mConfig.tag);
        props.put("lynxsdk_fluency_maximum_frames", rawMetrics.maximumFrames);

        // basic fluency info
        props.put("lynxsdk_fluency_frames_number", rawMetrics.frames);
        props.put("lynxsdk_fluency_fps", rawMetrics.fps);
        props.put("lynxsdk_fluency_dur", rawMetrics.duration);
        props.put("lynxsdk_fluency_drop1_count", rawMetrics.drop1);
        props.put("lynxsdk_fluency_drop1_duration", rawMetrics.drop1Duration);
        props.put("lynxsdk_fluency_drop3_count", rawMetrics.drop3);
        props.put("lynxsdk_fluency_drop3_duration", rawMetrics.drop3Duration);
        props.put("lynxsdk_fluency_drop7_count", rawMetrics.drop7);
        props.put("lynxsdk_fluency_drop7_duration", rawMetrics.drop7Duration);
        props.put("lynxsdk_fluency_drop25_count", rawMetrics.drop25);
        props.put("lynxsdk_fluency_drop25_duration", rawMetrics.drop25Duration);

        // frame loss ratio
        props.put("lynxsdk_fluency_drop1_count_per_second",
            1000.0 * rawMetrics.drop1 / rawMetrics.duration);
        props.put("lynxsdk_fluency_drop3_count_per_second",
            1000.0 * rawMetrics.drop3 / rawMetrics.duration);
        props.put("lynxsdk_fluency_drop7_count_per_second",
            1000.0 * rawMetrics.drop7 / rawMetrics.duration);
        props.put("lynxsdk_fluency_drop25_count_per_second",
            1000.0 * rawMetrics.drop25 / rawMetrics.duration);

        // frame loss time ratio
        props.put(
            "lynxsdk_fluency_drop1_ratio", 1000.0 * rawMetrics.drop1Duration / rawMetrics.duration);
        props.put(
            "lynxsdk_fluency_drop3_ratio", 1000.0 * rawMetrics.drop3Duration / rawMetrics.duration);
        props.put(
            "lynxsdk_fluency_drop7_ratio", 1000.0 * rawMetrics.drop7Duration / rawMetrics.duration);
        props.put("lynxsdk_fluency_drop25_ratio",
            1000.0 * rawMetrics.drop25Duration / rawMetrics.duration);

        // front throttle: enableLynxScrollFluency
        props.put("lynxsdk_fluency_pageconfig_probability", mConfig.pageConfigProbability);

        return props;
      };

      LynxEventReporter.onEvent(LYNXSDK_FLUENCY_EVENT, mInstanceId, builder);
    }
  }
}
