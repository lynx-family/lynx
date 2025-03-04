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

  private static final String TRACE_FETCHER_WRAPPER_USE_LYNX_RESOURCE_SERVICE =
      "Using LynxResourceServiceProvider";
  private static final String TRACE_FETCHER_WRAPPER_USE_LAZY_BUNDLE_FETCHER =
      "Using DynamicComponentFetcher";

  private AtomicBoolean mEnableLynxService = new AtomicBoolean(false);
  private LynxResourceServiceProvider mLynxServiceProvider = null;
  // dynamic component fetcher from client
  private DynamicComponentFetcher mDynamicComponentFetcher = null;

  public LynxExternalResourceFetcherWrapper(DynamicComponentFetcher fetcher) {
    mDynamicComponentFetcher = fetcher;
    if (LynxResourceServiceProvider.ensureLynxService()) {
      mLynxServiceProvider = new LynxResourceServiceProvider();
    }
  }

  public void SetEnableLynxResourceServiceProvider(boolean enable) {
    mEnableLynxService.set(enable);
  }

  // pass data and error
  public void fetchResourceWithHandler(String url, @NonNull LoadedHandler handler) {
    // firstly, try lynx resource service
    if (mEnableLynxService.get()) {
      TraceEvent.beginSection(TRACE_FETCHER_WRAPPER_USE_LYNX_RESOURCE_SERVICE);
      // if lynx resource service provider exists, use it, otherwise, try other fetchers
      if (mLynxServiceProvider != null) {
        mLynxServiceProvider.request(new LynxResourceRequest(url),
            new LynxResourceCallback<ILynxResourceResponseDataInfo>() {
              @Override
              public void onResponse(
                  @NonNull LynxResourceResponse<ILynxResourceResponseDataInfo> response) {
                // if lynx service exception, retry. Otherwise callback with error, do not retry.
                if (response.getCode()
                    == LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED) {
                  LLog.w(TAG, "Lynx service exception, retry with other fetchers, url: " + url);
                  fetchResourceWithDynamicComponentFetcher(url, handler);
                } else {
                  byte[] data = null;
                  if (null != response.getData()) {
                    data = response.getData().provideBytes();
                  }
                  handler.onLoaded(data, response.getError());
                }
              }
            });
        TraceEvent.endSection(TRACE_FETCHER_WRAPPER_USE_LYNX_RESOURCE_SERVICE);
        return;
      } else {
        // try other fetchers
        LLog.w(TAG,
            "LynxResourceServiceProvider is null, switch to the fetchers registered in by host. ");
      }
      TraceEvent.endSection(TRACE_FETCHER_WRAPPER_USE_LYNX_RESOURCE_SERVICE);
    }

    // if failed to launch a lynx service request, try other fetchers
    fetchResourceWithDynamicComponentFetcher(url, handler);
  }

  private void fetchResourceWithDynamicComponentFetcher(
      final String url, @NonNull LoadedHandler handler) {
    if (mDynamicComponentFetcher != null) {
      TraceEvent.beginSection(TRACE_FETCHER_WRAPPER_USE_LAZY_BUNDLE_FETCHER);
      mDynamicComponentFetcher.loadDynamicComponent(
          url, new DynamicComponentFetcher.LoadedHandler() {
            @Override
            public void onComponentLoaded(@Nullable byte[] data, @Nullable Throwable error) {
              handler.onLoaded(data, error);
            }
          });
      TraceEvent.endSection(TRACE_FETCHER_WRAPPER_USE_LAZY_BUNDLE_FETCHER);
      return;
    }

    // No available provider or fetcher
    handler.onLoaded(null, new Throwable("No available provider or fetcher"));
  }
}
