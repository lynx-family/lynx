// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import androidx.annotation.Keep;
import com.lynx.devtool.memory.MemoryController;
import com.lynx.devtool.tracing.FPSTrace;
import com.lynx.devtool.tracing.FrameViewTrace;
import com.lynx.devtool.tracing.InstanceTrace;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.TraceController;

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
  public static String getLynxVersion() {
    return LynxEnv.inst().getLynxVersion();
  }
}
