// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.image.model.ImageRequestInfo;

/** Allows an embedding application to customize and observe main image requests. */
public interface LynxImageRequestDelegate {
  /**
   * Returns the caller context for an image instance.
   *
   * <p>This method is called when the image instance is created. Return {@code null} to keep the
   * default caller context.
   */
  @Nullable
  default Object getImageCallerContext() {
    return null;
  }

  /**
   * Called before an image request is submitted.
   *
   * <p>The supplied {@link ImageRequestInfo} describes the original request. Return {@code null}
   * when no request-specific options are needed.
   */
  @Nullable
  default LynxImageRequestOptions prepareImageRequest(@NonNull ImageRequestInfo requestInfo) {
    return null;
  }

  /** Called once when the logical image request finishes. */
  default void onImageRequestFinished(
      @NonNull ImageRequestInfo requestInfo, @NonNull LynxImageRequestResult result) {}
}
