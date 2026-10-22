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
import com.lynx.devtool.tracing.MemoryTrace;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceController;
import com.lynx.tasm.utils.UIThreadUtils;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicBoolean;

@Keep
public class GlobalDevToolPlatformAndroidDelegate {
  private static final String TAG = "GlobalDevToolPlatformAndroidDelegate";
  private static final String EMPTY_RESULT_JSON = "{}";

  private static volatile long sHostScriptTransport;

  @CalledByNative
  private static long getHostScriptTransport() {
    return sHostScriptTransport;
  }

  /** Binds the transport supplied by the host's optional Host Script runtime. */
  public static void setHostScriptTransport(long transport) {
    sHostScriptTransport = transport;
  }

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
  public static void handleLynxSetting(String method, String key, String value, long callbackPtr) {
    AtomicBoolean didCallback = new AtomicBoolean(false);
    try {
      LynxSettingPlatformAndroid.handle(method, key, value, (resultJson, errorMessage) -> {
        if (didCallback.compareAndSet(false, true)) {
          nativeOnLynxSettingResult(callbackPtr, resultJson, errorMessage);
        }
      });
    } catch (Throwable throwable) {
      if (didCallback.compareAndSet(false, true)) {
        nativeOnLynxSettingResult(
            callbackPtr, EMPTY_RESULT_JSON, "LynxSetting failed: " + throwable.getMessage());
      }
    }
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
  public static long getMemoryTracePlugin() {
    return MemoryTrace.getInstance().getNativeMemoryTrace();
  }

  @CalledByNative
  public static String getLynxVersion() {
    return LynxEnv.inst().getLynxVersion();
  }

  public interface HSRScriptCallback {
    // Called on the UI thread. A null error means synchronous script loading completed.
    void onResult(String error);
  }

  /** Loads decoded schema parameters through the same boundary as HSR.loadScript. */
  public static void loadHSRScriptFromSchema(String paramsJson, HSRScriptCallback callback) {
    if (callback == null) {
      throw new IllegalArgumentException("HSR callback is required");
    }
    if (!LynxEnv.inst().isDevLibraryLoaded()) {
      UIThreadUtils.runOnUiThread(() -> callback.onResult("HSR DevTool library is not loaded"));
      return;
    }
    nativeLoadHSRScriptFromSchema(paramsJson.getBytes(StandardCharsets.UTF_8), callback);
  }

  @CalledByNative
  private static void onHSRScriptLoaded(Object callback, byte[] error) {
    String message = error == null ? null : new String(error, StandardCharsets.UTF_8);
    UIThreadUtils.runOnUiThread(() -> ((HSRScriptCallback) callback).onResult(message));
  }

  private static native void nativeLoadHSRScriptFromSchema(byte[] paramsJson, Object callback);

  @CalledByNative
  public static void fetchHSRScript(byte[] url, long callbackId) {
    HSRScriptSourceFetcher.fetch(new String(url, StandardCharsets.UTF_8),
        (source, error)
            -> nativeOnHSRScriptFetched(
                callbackId, source, error == null ? null : error.getBytes(StandardCharsets.UTF_8)));
  }

  private static native void nativeOnHSRScriptFetched(long callbackId, byte[] source, byte[] error);

  private static native void nativeOnMemoryUsageResult(
      long callbackPtr, String resultJson, String errorMessage);

  private static native void nativeOnLynxSettingResult(
      long callbackPtr, String resultJson, String errorMessage);
}
