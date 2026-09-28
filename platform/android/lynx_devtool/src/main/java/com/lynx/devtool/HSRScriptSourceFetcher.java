// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.provider.LynxProviderRegistry;
import com.lynx.tasm.provider.LynxResourceCallback;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.provider.LynxResourceRequest;
import com.lynx.tasm.provider.LynxResourceResponse;
import java.util.concurrent.atomic.AtomicBoolean;

final class HSRScriptSourceFetcher {
  interface Callback {
    void onResult(byte[] source, String error);
  }

  private HSRScriptSourceFetcher() {}

  static void fetch(String url, Callback callback) {
    AtomicBoolean completed = new AtomicBoolean();
    Callback once = (source, error) -> {
      if (completed.compareAndSet(false, true)) {
        callback.onResult(source, error);
      }
    };
    try {
      LynxResourceProvider<Object, byte[]> provider = LynxEnv.inst().getResourceProvider().get(
          LynxProviderRegistry.LYNX_PROVIDER_TYPE_EXTERNAL_JS);
      if (provider == null) {
        once.onResult(null, "HSR external JS resource provider is not configured");
        return;
      }
      // Providers may complete synchronously, including blocking file or network reads.
      LynxThreadPool.getBriefIOExecutor().execute(() -> fetch(provider, url, once));
    } catch (Throwable error) {
      once.onResult(null, errorMessage(error));
    }
  }

  private static void fetch(
      LynxResourceProvider<Object, byte[]> provider, String url, Callback callback) {
    try {
      // Leave URL interpretation and file access to the registered resource provider.
      provider.request(new LynxResourceRequest<>(url, null,
                           LynxResourceRequest.LynxResourceType.LynxResourceTypeExternalJSSource),
          new LynxResourceCallback<byte[]>() {
            @Override
            public void onResponse(LynxResourceResponse<byte[]> response) {
              if (response != null && response.success()) {
                callback.onResult(response.getData(), null);
              } else {
                callback.onResult(
                    null, errorMessage(response == null ? null : response.getError()));
              }
            }
          });
    } catch (Throwable error) {
      callback.onResult(null, errorMessage(error));
    }
  }

  private static String errorMessage(Throwable error) {
    if (error == null) {
      return "HSR source provider returned no script data";
    }
    String message = error.getMessage();
    return "Failed to load HSR script: "
        + (message == null || message.isEmpty() ? error.getClass().getSimpleName() : message);
  }
}
