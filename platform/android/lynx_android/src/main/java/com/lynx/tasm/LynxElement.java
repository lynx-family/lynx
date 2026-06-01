// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;

public final class LynxElement {
  private static final String TAG = "LynxElement";

  private final WeakReference<LynxTemplateRender> mTemplateRender;
  private final int mSign;

  /**
   * @apidoc
   *
   * @brief Callback used by asynchronous LynxElement APIs.
   * @note The callback is always invoked asynchronously on the UI thread.
   */
  public interface Callback<T> {
    void onResult(@Nullable T result);
  }

  LynxElement(@Nullable LynxTemplateRender templateRender, int sign) {
    mTemplateRender = new WeakReference<>(templateRender);
    mSign = sign;
  }

  /**
   * @apidoc
   *
   * @brief Asynchronously serializes this LynxElement tree to a JSON string.
   * @param callback Receives the JSON string, or null if the element is unavailable. The callback
   *     is always invoked asynchronously on the UI thread.
   */
  public void toJSONString(@NonNull final Callback<String> callback) {
    if (callback == null) {
      return;
    }
    LynxTemplateRender templateRender = mTemplateRender.get();
    if (templateRender == null || mSign == 0) {
      LLog.w(TAG, "toJSONString failed since native element is null.");
      dispatchOnUiThread(callback, null);
      return;
    }
    try {
      templateRender.lynxElementToJSONString(mSign, callback);
    } catch (RuntimeException e) {
      LLog.e(TAG, "toJSONString failed: " + e.getMessage());
      dispatchOnUiThread(callback, null);
    }
  }

  static <T> void dispatchOnUiThread(
      @NonNull final Callback<T> callback, @Nullable final T result) {
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        callback.onResult(result);
      }
    });
  }
}
