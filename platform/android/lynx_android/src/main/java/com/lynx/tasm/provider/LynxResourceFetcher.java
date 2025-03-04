// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.provider;

import androidx.annotation.AnyThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * Lynx Generic Resource Fetcher
 */
public abstract class LynxResourceFetcher<T> {
  private final static String TAG = "LynxResourceFetcher";
  /**
   * for resource request Async, must be overrode
   * @param request   request object, contain url and params
   * @param callback  callback when resource finish
   */
  @AnyThread
  @Nullable
  public abstract ILynxResourceRequestOperation request(
      @NonNull final LynxResourceRequest<T> request,
      @NonNull final LynxResourceCallback<ILynxResourceResponseDataInfo> callback);

  /**
   * for resource request Sync, for general lynx resource provider must be overrode
   * @param request   request object, contain url and params
   */
  @NonNull
  public abstract LynxResourceResponse<ILynxResourceResponseDataInfo> requestSync(
      @NonNull final LynxResourceRequest<T> request);
}
