// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image;

import androidx.annotation.Nullable;

/** The terminal result of a logical image request. */
public final class LynxImageRequestResult {
  private final int mErrorCode;
  private final @Nullable Throwable mThrowable;
  private final int mImageOrigin;

  public LynxImageRequestResult(int errorCode, @Nullable Throwable throwable) {
    this(errorCode, throwable, -1);
  }

  public LynxImageRequestResult(int errorCode, @Nullable Throwable throwable, int imageOrigin) {
    mErrorCode = errorCode;
    mThrowable = throwable;
    mImageOrigin = imageOrigin;
  }

  public int getErrorCode() {
    return mErrorCode;
  }

  @Nullable
  public Throwable getThrowable() {
    return mThrowable;
  }

  /** Returns the image loader's origin value, or -1 when unavailable. */
  public int getImageOrigin() {
    return mImageOrigin;
  }
}
