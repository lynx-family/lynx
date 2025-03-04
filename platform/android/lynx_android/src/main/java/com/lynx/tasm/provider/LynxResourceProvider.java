// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.provider;

import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.LynxContext;

/**
 * Lynx Standard Resource Provider
 */
public abstract class LynxResourceProvider<T, C> {
  /**
   * for resoure request, must be overrode
   * @param request   request object, contain url and params
   * @param callback  callback when resource finish
   */
  @AnyThread
  public void request(@NonNull final LynxResourceRequest<T> request,
      @NonNull final LynxResourceCallback<C> callback) {}

  @AnyThread
  public void request(@NonNull final LynxResourceRequest<T> request,
      @NonNull final LynxResourceCallback<C> callback, LynxContext contetx) {
    request(request, callback);
  }

  /**
   * Cancel request which has requested, default empty body. Called from Lynx
   * @param request witch will be cancelled
   */
  public void cancel(@NonNull final LynxResourceRequest<T> request) {}
}
