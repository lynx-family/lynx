// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.List;

public final class LynxElement {
  static final int REQUEST_ROOT_SIGN = 0;
  static final int REQUEST_TREE_JSON = 1;

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

  static final class NativeCallback extends PlatformCallBack {
    private final int mRequestType;
    private final LynxTemplateRender mTemplateRender;
    private final Callback mCallback;
    private final List<NativeCallback> mCallbackHolders;
    private boolean mCanceled;

    NativeCallback(int requestType, LynxTemplateRender templateRender, Callback callback,
        List<NativeCallback> callbackHolders) {
      mRequestType = requestType;
      mTemplateRender = templateRender;
      mCallback = callback;
      mCallbackHolders = callbackHolders;
    }

    @Override
    public void onSuccess(Object data) {
      UIThreadUtils.runOnUiThread(() -> {
        if (mCanceled) {
          return;
        }
        try {
          mCallback.onResult(getResult(data));
        } finally {
          mCallbackHolders.remove(this);
        }
      });
    }

    private Object getResult(Object data) {
      if (mRequestType == REQUEST_TREE_JSON) {
        if (!(data instanceof String)) {
          return null;
        }
        String json = (String) data;
        return json.isEmpty() ? null : json;
      }
      int sign = data instanceof Number ? ((Number) data).intValue() : 0;
      return sign != 0 ? new LynxElement(mTemplateRender, sign) : null;
    }

    void cancel() {
      if (mCanceled) {
        return;
      }
      mCanceled = true;
      try {
        mCallback.onResult(null);
      } finally {
        mCallbackHolders.remove(this);
      }
    }
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
      dispatchOnUiThread(callback, null);
      return;
    }
    templateRender.lynxElementToJSONString(mSign, callback);
  }

  static <T> void dispatchOnUiThread(
      @NonNull final Callback<T> callback, @Nullable final T result) {
    UIThreadUtils.runOnUiThread(() -> callback.onResult(result));
  }
}
