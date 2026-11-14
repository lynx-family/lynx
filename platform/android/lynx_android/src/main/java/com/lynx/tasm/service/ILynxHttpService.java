// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.jsbridge.network.HttpRequest;
import com.lynx.jsbridge.network.HttpStreamingDelegate;

@Keep
public interface ILynxHttpService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxHttpService.class;
  }

  void requestStreaming(@NonNull HttpRequest request,
      @NonNull final LynxHttpRequestCallback callback, @NonNull HttpStreamingDelegate delegate);

  void request(@NonNull HttpRequest request, @NonNull final LynxHttpRequestCallback callback);

  /**
   * Same as {@link #requestStreaming}, except that the request can be canceled. It is used by the
   * Fetch API, and the returned operation is canceled when the AbortSignal of the fetch is aborted.
   * The default implementation delegates to {@link #requestStreaming} and is not cancelable.
   *
   * @return An operation which can cancel the request, or null if cancel action is not supported.
   */
  @Nullable
  default ILynxHttpRequestOperation requestStreamingCancelable(@NonNull HttpRequest request,
      @NonNull final LynxHttpRequestCallback callback, @NonNull HttpStreamingDelegate delegate) {
    requestStreaming(request, callback, delegate);
    return null;
  }

  /**
   * Same as {@link #request}, except that the request can be canceled. It is used by the Fetch
   * API, and the returned operation is canceled when the AbortSignal of the fetch is aborted.
   * The default implementation delegates to {@link #request} and is not cancelable.
   *
   * @return An operation which can cancel the request, or null if cancel action is not supported.
   */
  @Nullable
  default ILynxHttpRequestOperation requestCancelable(
      @NonNull HttpRequest request, @NonNull final LynxHttpRequestCallback callback) {
    request(request, callback);
    return null;
  }
}
