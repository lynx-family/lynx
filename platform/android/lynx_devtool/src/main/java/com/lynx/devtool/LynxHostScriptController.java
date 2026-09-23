// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool;

import android.os.Handler;
import android.os.Looper;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.devtoolwrapper.CDPResultCallback;
import com.lynx.tasm.LynxEnv;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Internal, process-local control plane shipped in the DevTool release artifact.
 * The host must bootstrap DevTool before LynxEnv.init and supply matching SDK and
 * DevTool artifacts. This class never loads or replaces native libraries.
 * Callbacks receive a CDP result/error envelope on the Android main thread.
 */
@Keep
@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public final class LynxHostScriptController {
  private static final Handler MAIN = new Handler(Looper.getMainLooper());
  private static final AtomicLong NEXT_ID = new AtomicLong();

  private LynxHostScriptController() {}

  /** Uses the host's existing external-JS/template resource provider. */
  public static void start(@NonNull String url, @NonNull CDPResultCallback callback) {
    load("url", "url", url, callback);
  }

  /** Replaces the loaded script and evaluates its entry on BTS. */
  public static void startSource(@NonNull String source, @NonNull CDPResultCallback callback) {
    load("inline", "script", source, callback);
  }

  /**
   * Cancels pending commands and waits for runtime teardown on each owner.
   * Already-running JS is not interrupted. Evaluation is rejected until a new
   * load succeeds. This does not disable DevTool or its View prewarming hooks.
   */
  public static void stop(@NonNull CDPResultCallback callback) {
    invoke("HSR.stop", new JSONObject(), callback);
  }

  /** Reports availability, debug admission, per-domain readiness and load state. */
  public static void getStatus(@NonNull CDPResultCallback callback) {
    invoke("HSR.getStatus", new JSONObject(), callback);
  }

  private static void load(String type, String key, String value, CDPResultCallback callback) {
    JSONObject source = new JSONObject();
    JSONObject params = new JSONObject();
    try {
      source.put("type", type).put(key, value);
      params.put("source", source);
    } catch (JSONException error) {
      throw new IllegalArgumentException(error);
    }
    invoke("HSR.loadScript", params, callback);
  }

  private static void invoke(String method, JSONObject params, CDPResultCallback callback) {
    if (callback == null)
      throw new IllegalArgumentException("HSR callback is required");
    long id = NEXT_ID.incrementAndGet();
    AtomicBoolean completed = new AtomicBoolean();
    CDPResultCallback complete = result -> {
      if (completed.compareAndSet(false, true))
        MAIN.post(() -> callback.onResult(result));
    };
    try {
      LynxEnv env = LynxEnv.inst();
      if (!env.isInitCompleted() || !env.isDevLibraryLoaded()) {
        fail(id, "HSR_DEVTOOL_NOT_INITIALIZED", complete);
        return;
      }
      if (!env.getLynxVersion().equals(LynxDevtoolEnv.inst().getVersion())) {
        fail(id, "HSR_SDK_VERSION_MISMATCH", complete);
        return;
      }
      JSONObject message =
          new JSONObject().put("id", id).put("method", method).put("params", params);
      GlobalDevToolPlatformAndroidDelegate.nativeInvokeHSR(
          message.toString(), new CDPResultCallbackWrapper(complete));
    } catch (JSONException | RuntimeException | LinkageError error) {
      fail(id, "HSR_CONTROL_UNAVAILABLE: " + error, complete);
    }
  }

  private static void fail(long id, String message, CDPResultCallback callback) {
    try {
      JSONObject error = new JSONObject().put("code", -32000).put("message", message);
      callback.onResult(new JSONObject().put("id", id).put("error", error).toString());
    } catch (JSONException error) {
      throw new IllegalStateException(error);
    }
  }
}
