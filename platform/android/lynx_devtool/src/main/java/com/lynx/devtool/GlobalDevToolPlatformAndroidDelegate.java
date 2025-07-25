// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import androidx.annotation.Keep;
import com.lynx.devtool.helper.timingoverlay.TimingOverlayHelper;
import com.lynx.devtool.memory.MemoryController;
import com.lynx.devtool.tracing.FPSTrace;
import com.lynx.devtool.tracing.FrameViewTrace;
import com.lynx.devtool.tracing.InstanceTrace;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.TraceController;
import com.lynx.tasm.performance.fsptracing.FSPConfig;
import com.lynx.tasm.performance.fsptracing.FSPTracer;
import com.lynx.tasm.performance.fsptracing.area.FSPAreaConfig;
import com.lynx.tasm.performance.fsptracing.axial.FSPAxialConfig;
import org.json.JSONObject;

@Keep
public class GlobalDevToolPlatformAndroidDelegate {
  @CalledByNative
  public static void startMemoryTracing() {
    MemoryController.getInstance().startMemoryTracing();
  }

  @CalledByNative
  public static void stopMemoryTracing() {
    MemoryController.getInstance().stopMemoryTracing();
  }

  @CalledByNative
  public static long getTraceController() {
    return TraceController.getInstance().getNativeTraceController();
  }

  @CalledByNative
  public static long getFPSTracePlugin() {
    return FPSTrace.getInstance().getNativeFPSTrace();
  }

  @CalledByNative
  public static long getFrameViewTracePlugin() {
    return FrameViewTrace.getInstance().getNativeFrameViewTrace();
  }

  @CalledByNative
  public static long getInstanceTracePlugin() {
    return InstanceTrace.getInstance().getNativeInstanceTrace();
  }

  @CalledByNative
  public static void setTimingOverlay(boolean enabled, String paramsJson) {
    TimingOverlayHelper.setEnabled(enabled);

    if (!enabled) {
      FSPTracer.setEnabled(false, null);
      return;
    }

    FSPConfig config = null;
    try {
      JSONObject params = new JSONObject(paramsJson);
      int fspMode = params.has("fsp_mode") ? params.getInt("fsp_mode") : 0;
      if (fspMode == 1) {
        FSPAxialConfig cfg = new FSPAxialConfig();
        if (params.has("fsp_acceptable_difference_per_ms")) {
          double fspAcceptableDifferencePerMs =
              params.getDouble("fsp_acceptable_difference_per_ms");
          cfg.setAcceptablePixelDifferencePerMs(fspAcceptableDifferencePerMs);
        }
        if (params.has("fsp_acceptable_difference")) {
          double fspAcceptableDifference = params.getDouble("fsp_acceptable_difference");
          cfg.setAcceptablePixelDifferencePerSnapshot(fspAcceptableDifference);
        }
        if (params.has("fsp_min_completion_rate_1")) {
          double fspMinCompletionRate1 = params.getDouble("fsp_min_completion_rate_1");
          cfg.setMinimumCompletionRateX(fspMinCompletionRate1);
        }
        if (params.has("fsp_min_completion_rate_2")) {
          double fspMinCompletionRate2 = params.getDouble("fsp_min_completion_rate_2");
          cfg.setMinimumCompletionRateY(fspMinCompletionRate2);
        }
        config = cfg;
      } else {
        FSPAreaConfig cfg = new FSPAreaConfig();
        if (params.has("fsp_acceptable_difference_per_ms")) {
          double fspAcceptableDifferencePerMs =
              params.getDouble("fsp_acceptable_difference_per_ms");
          cfg.setAcceptableDifferencePerMs(fspAcceptableDifferencePerMs);
        }
        if (params.has("fsp_acceptable_difference")) {
          double fspAcceptableDifference = params.getDouble("fsp_acceptable_difference");
          cfg.setAcceptableDifferencePerSnapshot(fspAcceptableDifference);
        }
        if (params.has("fsp_min_completion_rate_1")) {
          double fspMinCompletionRate1 = params.getDouble("fsp_min_completion_rate_1");
          cfg.setMinimumCompletionRate(fspMinCompletionRate1);
        }
        config = cfg;
      }

      if (params.has("fsp_interval")) {
        double fspInterval = params.getDouble("fsp_interval");
        config.setSnapshotInterval(fspInterval);
      }
      if (params.has("fsp_hard_timeout")) {
        double fspHardTimeout = params.getDouble("fsp_hard_timeout");
        config.setHardTimeout(fspHardTimeout);
      }
      if (params.has("fsp_graceful_timeout")) {
        double fspGracefulTimeout = params.getDouble("fsp_graceful_timeout");
        config.setGracefulTimeout(fspGracefulTimeout);
      }
      if (params.has("fsp_extended_graceful_timeout")) {
        double fspExtendedGracefulTimeout = params.getDouble("fsp_extended_graceful_timeout");
        config.setExtendedGracefulTimeout(fspExtendedGracefulTimeout);
      }
    } catch (Exception e) {
    }

    FSPTracer.setEnabled(enabled, config);
  }

  @CalledByNative
  public static String getLynxVersion() {
    return LynxEnv.inst().getLynxVersion();
  }
}
