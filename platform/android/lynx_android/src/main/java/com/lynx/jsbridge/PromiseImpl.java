/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

/**
 * Implementation of two javascript functions that can be used to resolve or reject a js promise.
 */
package com.lynx.jsbridge;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.WritableMap;

@Keep
public class PromiseImpl implements Promise {
  private static final String DEFAULT_ERROR = "EUNSPECIFIED";

  private @Nullable Callback mResolve;
  private @Nullable Callback mReject;

  @Keep
  public PromiseImpl(@Nullable Callback resolve, @Nullable Callback reject) {
    mResolve = resolve;
    mReject = reject;
  }

  @Keep
  @Override
  public void resolve(Object value) {
    if (mResolve != null) {
      mResolve.invoke(value);
    }
  }

  @Keep
  @Override
  @Deprecated
  public void reject(String message) {
    reject(DEFAULT_ERROR, message);
  }

  @Keep
  @Override
  public void reject(String code, String message) {
    if (mReject != null) {
      if (code == null) {
        code = DEFAULT_ERROR;
      }
      WritableMap errorInfo = new JavaOnlyMap();
      errorInfo.putString("code", code);
      errorInfo.putString("message", message);
      mReject.invoke(errorInfo);
    }
  }
}
