// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.content.Context;
import androidx.annotation.AnyThread;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.utils.UIThreadUtils;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;

/**
 * A debug-only runtime that executes one Host Script and exposes runtime-local Lynx capabilities.
 */
@Keep
public final class LynxHostScriptRuntime {
  public interface ScriptCallback {
    void onSuccess();
    void onFailure(@NonNull LynxError error);
  }

  public static final String ERROR_INVALID_STATE = "INVALID_STATE";
  public static final String ERROR_INVALID_ENTRY = "INVALID_ENTRY";
  public static final String ERROR_SCRIPT_EVALUATION = "SCRIPT_EVALUATION";
  public static final String ERROR_ENTRY_REJECTED = "ENTRY_REJECTED";
  public static final String ERROR_SOURCE_LOAD = "SOURCE_LOAD";
  public static final String ERROR_INFO_KEY = "host_script_error_code";

  private static final int STATE_CREATED = 0;
  private static final int STATE_LOADING = 1;
  private static final int STATE_STARTING = 2;
  private static final int STATE_READY = 3;
  private static final int STATE_FAILED = 4;
  private static final int STATE_DESTROYED = 5;
  private final Object mStateLock = new Object();
  private final LynxBackgroundRuntime mRuntime;
  private final LynxHostScriptRuntimeBinding mRuntimeBinding;
  private final LynxHostScriptLynxViewBinding mLynxViewBinding;
  private final LynxBackgroundRuntimeClient mRuntimeClient;
  private final @Nullable LynxGenericResourceFetcher mSourceFetcher;

  private int mState = STATE_CREATED;
  private boolean mEntryRegistered;
  private boolean mEntryReady;
  private boolean mStartEvaluationEnded;
  private String mStartUrl;
  private ScriptCallback mStartCallback;
  private String mEvaluationUrl;
  private ScriptCallback mEvaluationCallback;
  private LynxResourceRequest mPendingSourceRequest;
  private boolean mLynxViewBound;

  @NonNull
  public static LynxHostScriptRuntime create(
      @NonNull Context context, @NonNull LynxBackgroundRuntimeOptions options, boolean debuggable) {
    if (context == null || options == null) {
      throw new IllegalArgumentException("Context and options must not be null");
    }
    if (!UIThreadUtils.isOnUiThread()) {
      throw new IllegalStateException("LynxHostScriptRuntime.create must run on the UI thread");
    }
    return new LynxHostScriptRuntime(
        context, new LynxBackgroundRuntimeOptions(options), debuggable);
  }

  private LynxHostScriptRuntime(
      Context context, LynxBackgroundRuntimeOptions options, boolean debuggable) {
    mRuntimeClient = new LynxBackgroundRuntimeClient() {
      @Override
      public void onReceivedError(LynxError error) {
        handleRuntimeError(error);
      }

      @Override
      public void onEvaluateJavaScriptEnd(String url) {
        handleEvaluationEnd(url);
      }
    };
    mRuntimeBinding = new LynxHostScriptRuntimeBinding(this::handleHostScriptResult);
    mLynxViewBinding = new LynxHostScriptLynxViewBinding(mRuntimeBinding);
    mSourceFetcher = options.getGenericResourceFetcher();
    mRuntime = new LynxBackgroundRuntime(context, options, debuggable);
    if (mRuntime.getState() != LynxBackgroundRuntime.STATE_START) {
      mLynxViewBinding.destroy();
      mRuntimeBinding.destroy();
      throw new IllegalStateException("Cannot create Host Script BackgroundRuntime");
    }
    mRuntime.addLynxBackgroundRuntimeClient(mRuntimeClient);
    mRuntime.addRuntimeLifecycleListener(mRuntimeBinding);
  }

  /** Binds the single platform-owned LynxView capability for this runtime. */
  public void bindLynxView(@NonNull LynxView lynxView) {
    if (lynxView == null) {
      throw new IllegalArgumentException("LynxView must not be null");
    }
    if (!UIThreadUtils.isOnUiThread()) {
      throw new IllegalStateException("bindLynxView must run on the UI thread");
    }
    synchronized (mStateLock) {
      if (mState == STATE_DESTROYED) {
        throw new IllegalStateException("Host Script Runtime was destroyed");
      }
      if (mLynxViewBound) {
        throw new IllegalStateException("Host Script Runtime can bind exactly one LynxView");
      }
      if (!mLynxViewBinding.bindView(lynxView)) {
        throw new IllegalStateException("Cannot bind Host Script Runtime to LynxView");
      }
      mLynxViewBound = true;
    }
  }

  /** Loads and executes the single standard Host Script entry for this runtime. */
  @AnyThread
  public void startJavaScript(@NonNull String sourceUrl, @NonNull ScriptCallback callback) {
    if (sourceUrl == null || sourceUrl.isEmpty() || callback == null) {
      postFailure(callback, ERROR_INVALID_STATE, "Source URL and callback are required");
      return;
    }

    LynxResourceRequest request = new LynxResourceRequest(
        sourceUrl, LynxResourceRequest.LynxResourceType.LynxResourceTypeExternalJSSource);
    request.setAsyncMode(LynxResourceRequest.AsyncMode.EXACTLY_ASYNC);
    synchronized (mStateLock) {
      if (mState != STATE_CREATED) {
        postFailure(
            callback, ERROR_INVALID_STATE, "Host Script Runtime can start exactly one main script");
        return;
      }
      mState = STATE_LOADING;
      mStartCallback = callback;
      mPendingSourceRequest = request;
    }

    if (mSourceFetcher == null) {
      handleSourceFailure(request, "No generic resource fetcher is configured");
      return;
    }
    try {
      mSourceFetcher.fetchResource(
          request, response -> handleSourceResponse(request, sourceUrl, response));
    } catch (Throwable throwable) {
      handleSourceFailure(request, "Failed to load Host Script source: " + throwable);
    }
  }

  /** Executes the single standard Host Script entry for this runtime. */
  @AnyThread
  public void startJavaScript(
      @NonNull String sourceUrl, @NonNull String source, @NonNull ScriptCallback callback) {
    if (sourceUrl == null || sourceUrl.isEmpty() || source == null || callback == null) {
      postFailure(callback, ERROR_INVALID_STATE, "Source URL, source and callback are required");
      return;
    }
    String runtimeUrl = normalizeSourceUrl(sourceUrl);
    synchronized (mStateLock) {
      if (mState != STATE_CREATED) {
        postFailure(
            callback, ERROR_INVALID_STATE, "Host Script Runtime can start exactly one main script");
        return;
      }
      mState = STATE_STARTING;
      mStartUrl = runtimeUrl;
      mStartCallback = callback;
    }
    mRuntime.evaluateJavaScript(runtimeUrl, wrapSource(source));
  }

  /** Executes an additional raw script after the standard Host Script is ready. */
  @AnyThread
  public void evaluateJavaScript(
      @NonNull String sourceUrl, @NonNull String source, @NonNull ScriptCallback callback) {
    if (sourceUrl == null || sourceUrl.isEmpty() || source == null || callback == null) {
      postFailure(callback, ERROR_INVALID_STATE, "Source URL, source and callback are required");
      return;
    }
    String runtimeUrl = normalizeSourceUrl(sourceUrl);
    synchronized (mStateLock) {
      if (mState != STATE_READY || mEvaluationCallback != null) {
        postFailure(callback, ERROR_INVALID_STATE,
            "Raw evaluation requires a ready runtime and no pending evaluation");
        return;
      }
      mEvaluationUrl = runtimeUrl;
      mEvaluationCallback = callback;
    }
    mRuntime.evaluateJavaScript(runtimeUrl, wrapSource(source));
  }

  /** Destroys this runtime. This operation is idempotent. */
  @AnyThread
  public void destroy() {
    ScriptCallback startCallback;
    ScriptCallback evaluationCallback;
    LynxResourceRequest sourceRequest;
    synchronized (mStateLock) {
      if (mState == STATE_DESTROYED) {
        return;
      }
      mState = STATE_DESTROYED;
      startCallback = takeStartCallbackLocked();
      evaluationCallback = takeEvaluationCallbackLocked();
      sourceRequest = mPendingSourceRequest;
      mPendingSourceRequest = null;
    }
    if (sourceRequest != null && mSourceFetcher != null) {
      try {
        mSourceFetcher.cancel(sourceRequest);
      } catch (Throwable ignored) {
        // Destruction must continue even if a host fetcher cannot cancel cleanly.
      }
    }
    if (startCallback != null) {
      postFailure(startCallback, ERROR_INVALID_STATE, "Host Script Runtime was destroyed");
    }
    if (evaluationCallback != null) {
      postFailure(evaluationCallback, ERROR_INVALID_STATE, "Host Script Runtime was destroyed");
    }
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      mLynxViewBinding.destroy();
      mRuntime.removeLynxBackgroundRuntimeClient(mRuntimeClient);
      mRuntime.destroy();
      mRuntimeBinding.destroy();
    });
  }

  private void handleSourceResponse(LynxResourceRequest request, String sourceUrl,
      @Nullable LynxResourceResponse<byte[]> response) {
    if (response == null || response.getState() != LynxResourceResponse.ResponseState.SUCCESS) {
      Throwable error = response == null ? null : response.getError();
      handleSourceFailure(request,
          error == null ? "Failed to load Host Script source"
                        : "Failed to load Host Script source: " + error);
      return;
    }

    String source;
    try {
      source = decodeSource(response.getData());
    } catch (CharacterCodingException exception) {
      handleSourceFailure(request, "Host Script source is not valid UTF-8");
      return;
    }
    if (source.isEmpty()) {
      handleSourceFailure(request, "Host Script source is empty");
      return;
    }

    String runtimeUrl = normalizeSourceUrl(sourceUrl);
    synchronized (mStateLock) {
      if (mState != STATE_LOADING || mPendingSourceRequest != request) {
        return;
      }
      mPendingSourceRequest = null;
      mState = STATE_STARTING;
      mStartUrl = runtimeUrl;
    }
    mRuntime.evaluateJavaScript(runtimeUrl, wrapSource(source));
  }

  private void handleSourceFailure(LynxResourceRequest request, String message) {
    ScriptCallback callback;
    synchronized (mStateLock) {
      if (mState != STATE_LOADING || mPendingSourceRequest != request) {
        return;
      }
      mPendingSourceRequest = null;
      callback = failStartLocked();
    }
    if (callback != null) {
      postFailure(callback, ERROR_SOURCE_LOAD, message);
    }
  }

  private static String decodeSource(@Nullable byte[] bytes) throws CharacterCodingException {
    if (bytes == null || bytes.length == 0) {
      return "";
    }
    int offset = bytes.length >= 3 && (bytes[0] & 0xff) == 0xef && (bytes[1] & 0xff) == 0xbb
            && (bytes[2] & 0xff) == 0xbf
        ? 3
        : 0;
    return StandardCharsets.UTF_8.newDecoder()
        .onMalformedInput(CodingErrorAction.REPORT)
        .onUnmappableCharacter(CodingErrorAction.REPORT)
        .decode(ByteBuffer.wrap(bytes, offset, bytes.length - offset))
        .toString();
  }

  private void handleHostScriptResult(String status, @Nullable String message) {
    ScriptCallback callback = null;
    LynxError error = null;
    synchronized (mStateLock) {
      if (mState != STATE_STARTING) {
        return;
      }
      if ("REGISTERED".equals(status)) {
        if (mEntryRegistered) {
          error = createError(ERROR_INVALID_ENTRY, "defineHostScript was called more than once");
        } else {
          mEntryRegistered = true;
        }
      } else if ("READY".equals(status)) {
        mEntryReady = true;
      } else if ("ERROR".equals(status)) {
        error = createError(ERROR_ENTRY_REJECTED,
            message == null || message.isEmpty() ? "Host Script entry rejected" : message);
      } else if ("INVALID_ENTRY".equals(status)) {
        error = createError(ERROR_INVALID_ENTRY,
            message == null || message.isEmpty() ? "Host Script entry is invalid" : message);
      } else {
        error = createError(ERROR_INVALID_ENTRY, "Unknown Host Script entry status");
      }

      if (error != null) {
        callback = failStartLocked();
      } else {
        callback = finishStartLocked();
      }
    }
    if (callback != null) {
      if (error == null) {
        postSuccess(callback);
      } else {
        postFailure(callback, error);
      }
    }
  }

  private void handleEvaluationEnd(String url) {
    ScriptCallback success = null;
    ScriptCallback failure = null;
    LynxError error = null;
    synchronized (mStateLock) {
      if (mState == STATE_STARTING && mStartUrl != null && mStartUrl.equals(url)) {
        mStartEvaluationEnded = true;
        if (!mEntryRegistered) {
          error = createError(
              ERROR_INVALID_ENTRY, "Host Script did not call defineHostScript synchronously");
          failure = failStartLocked();
        } else {
          success = finishStartLocked();
        }
      } else if (mState == STATE_READY && mEvaluationCallback != null && mEvaluationUrl != null
          && mEvaluationUrl.equals(url)) {
        success = takeEvaluationCallbackLocked();
      }
    }
    if (success != null) {
      postSuccess(success);
    }
    if (failure != null) {
      postFailure(failure, error);
    }
  }

  private void handleRuntimeError(LynxError runtimeError) {
    ScriptCallback callback = null;
    synchronized (mStateLock) {
      if (mState == STATE_STARTING) {
        callback = failStartLocked();
      } else if (mState == STATE_READY && mEvaluationCallback != null) {
        callback = takeEvaluationCallbackLocked();
      }
    }
    if (callback != null) {
      LynxError error = createError(ERROR_SCRIPT_EVALUATION,
          runtimeError == null ? "Host Script evaluation failed"
                               : runtimeError.getSummaryMessage());
      postFailure(callback, error);
    }
  }

  private ScriptCallback finishStartLocked() {
    if (!mEntryReady || !mStartEvaluationEnded) {
      return null;
    }
    mState = STATE_READY;
    return takeStartCallbackLocked();
  }

  private ScriptCallback takeEvaluationCallbackLocked() {
    ScriptCallback callback = mEvaluationCallback;
    mEvaluationCallback = null;
    mEvaluationUrl = null;
    return callback;
  }

  private ScriptCallback failStartLocked() {
    mState = STATE_FAILED;
    return takeStartCallbackLocked();
  }

  private ScriptCallback takeStartCallbackLocked() {
    ScriptCallback callback = mStartCallback;
    mStartCallback = null;
    return callback;
  }

  private static LynxError createError(String code, String message) {
    LynxError error = new LynxError(LynxSubErrorCode.E_BTS_RUNTIME_ERROR_SCRIPT_ERROR, message);
    error.addCustomInfo(ERROR_INFO_KEY, code);
    return error;
  }

  // BackgroundRuntime resolves inline standalone entries by absolute bundle path. Preserve an
  // existing path and normalize every other source identifier exactly once.
  private static String normalizeSourceUrl(String sourceUrl) {
    return sourceUrl.startsWith("/") ? sourceUrl : "/" + sourceUrl;
  }

  // BackgroundRuntime reads the standalone entry from global initBundle. Adapt ordinary Host
  // Script source to that contract and expose its runtime-local Lynx object as the lexical `lynx`
  // binding expected by Host Script source.
  private static String wrapSource(String source) {
    return "(function(){var g=(0,eval)('this');g.initBundle="
        + "function(__hostScriptRuntime){var lynx=__hostScriptRuntime.tt.lynx;\n" + source
        + "\n};})()";
  }

  private static void postFailure(@Nullable ScriptCallback callback, String code, String message) {
    if (callback == null) {
      return;
    }
    postFailure(callback, createError(code, message));
  }

  private static void postSuccess(@NonNull ScriptCallback callback) {
    UIThreadUtils.runOnUiThreadImmediately(callback::onSuccess);
  }

  private static void postFailure(@NonNull ScriptCallback callback, @NonNull LynxError error) {
    UIThreadUtils.runOnUiThreadImmediately(() -> callback.onFailure(error));
  }
}
