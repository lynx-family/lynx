// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import android.util.Log;
import androidx.annotation.Keep;
import com.lynx.devtool.memory.MemoryController;
import com.lynx.devtool.memory.MemoryUsageResultSerializer;
import com.lynx.devtool.tracing.FPSTrace;
import com.lynx.devtool.tracing.FrameViewTrace;
import com.lynx.devtool.tracing.InstanceTrace;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceController;
import java.util.concurrent.atomic.AtomicBoolean;

@Keep
public class GlobalDevToolPlatformAndroidDelegate {
  private static final String TAG = "GlobalDevToolPlatformAndroidDelegate";
  private static final String EMPTY_RESULT_JSON = "{}";

  @CalledByNative
  public static void startMemoryTracing() {
    MemoryController.getInstance().startMemoryTracing();
  }

  @CalledByNative
  public static void stopMemoryTracing() {
    MemoryController.getInstance().stopMemoryTracing();
  }

  @CalledByNative
  public static void queryAllMemoryUsage(long timeoutMs, long callbackPtr) {
    AtomicBoolean didCallback = new AtomicBoolean(false);
    try {
      MemoryUsageResultSerializer.queryAllMemoryUsage(
          timeoutMs, new MemoryUsageResultSerializer.ResultCallback() {
            @Override
            public void onResult(String resultJson, String errorMessage) {
              completeMemoryUsageQuery(callbackPtr, didCallback, resultJson, errorMessage);
            }
          });
    } catch (Throwable throwable) {
      completeMemoryUsageQuery(
          callbackPtr, didCallback, EMPTY_RESULT_JSON, memoryUsageQueryFailure(throwable));
    }
  }

  private static void completeMemoryUsageQuery(
      long callbackPtr, AtomicBoolean didCallback, String resultJson, String errorMessage) {
    if (didCallback.compareAndSet(false, true)) {
      try {
        nativeOnMemoryUsageResult(callbackPtr, resultJson, errorMessage);
      } catch (Throwable throwable) {
        LLog.e(TAG,
            "Failed to deliver Lynx memory usage result: " + Log.getStackTraceString(throwable));
      }
    }
  }

  private static String memoryUsageQueryFailure(Throwable throwable) {
    String message = throwable.getMessage();
    if (message == null || message.isEmpty()) {
      message = throwable.getClass().getSimpleName();
    }
    return "Failed to query Lynx memory usage: " + message;
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

  private static native void nativeOnMemoryUsageResult(
      long callbackPtr, String resultJson, String errorMessage);
}
