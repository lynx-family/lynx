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
import com.lynx.devtoolwrapper.DevToolLifecycle;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceController;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.provider.AbsTemplateProvider;
import com.lynx.tasm.provider.LynxProviderRegistry;
import com.lynx.tasm.provider.LynxResourceCallback;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.provider.LynxResourceRequest;
import com.lynx.tasm.provider.LynxResourceResponse;
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

  // Resource adaptation only; the native owner coordinates replacement and completion.
  @CalledByNative
  private static void fetchHSRScript(String url, long requestId) {
    if (!DevToolLifecycle.getInstance().isEnabled()) {
      nativeOnHSRScriptSource(requestId, null, "HSR_DEBUG_DISABLED");
      return;
    }
    try {
      LynxThreadPool.getBriefIOExecutor().execute(() -> fetchHSRScriptOnIO(url, requestId));
    } catch (RuntimeException error) {
      nativeOnHSRScriptSource(requestId, null, error.toString());
    }
  }

  private static void fetchHSRScriptOnIO(String url, long requestId) {
    if (!DevToolLifecycle.getInstance().isEnabled()) {
      nativeOnHSRScriptSource(requestId, null, "HSR_DEBUG_DISABLED");
      return;
    }
    AtomicBoolean completed = new AtomicBoolean(false);
    AbsTemplateProvider.Callback callback = new AbsTemplateProvider.Callback() {
      @Override
      public void onSuccess(byte[] bytes) {
        if (!DevToolLifecycle.getInstance().isEnabled()) {
          onFailed("HSR_DEBUG_DISABLED");
          return;
        }
        if (completed.compareAndSet(false, true)) {
          nativeOnHSRScriptSource(requestId, bytes, null);
        }
      }
      @Override
      public void onFailed(String error) {
        if (completed.compareAndSet(false, true)) {
          nativeOnHSRScriptSource(
              requestId, null, error == null ? "HSR resource request failed" : error);
        }
      }
    };
    try {
      LynxResourceProvider provider = LynxEnv.inst().getResourceProvider().get(
          LynxProviderRegistry.LYNX_PROVIDER_TYPE_EXTERNAL_JS);
      if (provider != null) {
        provider.request(new LynxResourceRequest(url), new LynxResourceCallback<byte[]>() {
          @Override
          public void onResponse(LynxResourceResponse<byte[]> response) {
            if (response != null && response.success()) {
              callback.onSuccess(response.getData());
            } else {
              callback.onFailed(response != null && response.getError() != null
                      ? response.getError().toString()
                      : "HSR resource request failed");
            }
          }
        });
      } else if (LynxEnv.inst().getTemplateProvider() != null) {
        // The existing host provider owns URL schemes; do not rewrite the URL.
        LynxEnv.inst().getTemplateProvider().loadTemplate(url, callback);
      } else {
        callback.onFailed("No HSR resource provider is installed");
      }
    } catch (RuntimeException error) {
      callback.onFailed(error.toString());
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

  private static native void nativeOnHSRScriptSource(
      long requestId, byte[] source, String errorMessage);

  private static native void nativeOnMemoryUsageResult(
      long callbackPtr, String resultJson, String errorMessage);

  private static native void nativeOnLynxSettingResult(
      long callbackPtr, String resultJson, String errorMessage);
}
