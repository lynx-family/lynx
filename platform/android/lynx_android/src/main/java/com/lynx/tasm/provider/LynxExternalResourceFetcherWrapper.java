// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.provider;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.component.DynamicComponentFetcher;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Lynx External Resource Fetcher Wrapper
 * Provide an unified external resource loading interface by managing fetchers and providers
 * TODO(zhoupeng.z): support for more types of resource requests
 * TODO(zhoupeng.z): consider to remove this wrapper after the lynx resource service fetcher is
 * stable or deprecated
 */
public class LynxExternalResourceFetcherWrapper {
  public interface LoadedHandler {
    void onLoaded(@Nullable byte[] data, @Nullable Throwable error);
  }

  private final static String TAG = "LynxExternalResourceFetcherWrapper";

  private LynxResourceServiceProvider mLynxServiceProvider = null;
  // dynamic component fetcher from client
  private DynamicComponentFetcher mDynamicComponentFetcher = null;

  public LynxExternalResourceFetcherWrapper(DynamicComponentFetcher fetcher) {
    mDynamicComponentFetcher = fetcher;
  }

  public boolean fetchResourceWithDynamicComponentFetcher(
      final String url, @NonNull LoadedHandler handler) {
    if (mDynamicComponentFetcher != null) {
      TraceEvent.beginSection(TraceEventDef.FETCHER_WRAPPER_USE_LAZY_BUNDLE_FETCHER);
      mDynamicComponentFetcher.loadDynamicComponent(
          url, new DynamicComponentFetcher.LoadedHandler() {
            @Override
            public void onComponentLoaded(@Nullable byte[] data, @Nullable Throwable error) {
              handler.onLoaded(data, error);
            }
          });
      TraceEvent.endSection(TraceEventDef.FETCHER_WRAPPER_USE_LAZY_BUNDLE_FETCHER);
      return true;
    }
    return false;
  }
}
