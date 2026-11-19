// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.core.resource;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;

/** Thin engine-side bridge for the optional Host Script bundle fetch hook. */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public final class LynxHostScriptResourceHooks {
  private static volatile Delegate sDelegate;

  private LynxHostScriptResourceHooks() {}

  /** Callback used by a registered delegate to resume or answer a bundle fetch. */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public interface Callback {
    void onMock(byte[] data);

    void onProceed(@Nullable String url);

    void onError(@Nullable String message);
  }

  /** Implementation supplied by the optional Host Script library on install. */
  @RestrictTo(RestrictTo.Scope.LIBRARY)
  public interface Delegate {
    boolean hasHandler(long viewId);

    void dispatch(
        long viewId, @NonNull String url, boolean reqInCurThread, @NonNull Callback callback);
  }

  public static void setDelegate(@Nullable Delegate delegate) {
    sDelegate = delegate;
  }

  @Nullable
  public static Delegate getDelegate() {
    return sDelegate;
  }

  /**
   * Takes over a lazy or frame bundle request when a Host Script delegate has a handler for its
   * View. Returns false to leave the request to the original path untouched.
   */
  static boolean intercept(final LynxResourceLoader loader, long viewId, final long responseHandler,
      final String url, final int type, boolean reqInCurThread) {
    Delegate delegate = sDelegate;
    if (delegate == null
        || (type != LynxResourceType.LYNX_RESOURCE_TYPE_JS_LAZY_BUNDLE
            && type != LynxResourceType.LYNX_RESOURCE_TYPE_FRAME)
        || url == null || !delegate.hasHandler(viewId)) {
      return false;
    }
    delegate.dispatch(viewId, url, reqInCurThread, new Callback() {
      @Override
      public void onMock(byte[] data) {
        loader.deliverMockedBundle(responseHandler, url, type, data);
      }

      @Override
      public void onProceed(String resolvedUrl) {
        loader.loadResource(responseHandler, resolvedUrl != null ? resolvedUrl : url, type);
      }

      @Override
      public void onError(String message) {
        // to release native pointer if exception occurs
        LynxResourceLoader.InvokeNativeCallbackWithBytes(
            responseHandler, null, LynxResourceLoader.RESOURCE_LOADER_FAILED, message);
      }
    });
    return true;
  }
}
