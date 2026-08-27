// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image;

import androidx.annotation.Nullable;

/** The terminal result of a logical image request. */
public final class LynxImageRequestResult {
  private final int mErrorCode;
  private final @Nullable Throwable mThrowable;

  public LynxImageRequestResult(int errorCode, @Nullable Throwable throwable) {
    mErrorCode = errorCode;
    mThrowable = throwable;
  }

  public int getErrorCode() {
    return mErrorCode;
  }

  @Nullable
  public Throwable getThrowable() {
    return mThrowable;
  }
}
