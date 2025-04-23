// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core.resource;

import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.LLog;
import java.lang.ref.WeakReference;

/**
 * Provide unified external script loading callback
 */
class ExternalScriptResourceCallback extends GuardedResourceCallback {
  private final long mResponseHandler;
  private WeakReference<LynxResourceLoader> weakLoader;

  ExternalScriptResourceCallback(LynxResourceLoader loader, String url, long responseHandler) {
    super(url);
    this.weakLoader = new WeakReference<>(loader);
    this.mResponseHandler = responseHandler;
  }

  public void onScriptLoaded(boolean success, byte[] data, String errorMsg) {
    if (!EnsureInvokedOnce()) {
      // Ensure responseHandler should be invoked just once.
      return;
    }
    int errCode = LynxResourceLoader.RESOURCE_LOADER_SUCCESS;
    String errMsg = null;
    String rootCause = null;
    if (success) {
      LLog.i(LynxResourceLoader.TAG, "loadExternalResourceAsync onSuccess.");
      if (data == null || data.length == 0) {
        errCode = LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED;
        errMsg = LynxResourceLoader.MSG_NULL_DATA;
      }
      LynxResourceLoader.InvokeNativeCallbackWithBytes(mResponseHandler, data, errCode, errMsg);
    } else {
      errCode = LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED;
      errMsg = "Error when fetch script";
      rootCause = errMsg;
      LynxResourceLoader.InvokeNativeCallbackWithBytes(
          mResponseHandler, null, errCode, errMsg + ": " + rootCause);
    }
    // Report only when loading script, the lazy bundle error will be reported in C++
    // TODO(zhoupeng.z): Report script error in C++
    if (errCode != LynxResourceLoader.RESOURCE_LOADER_SUCCESS) {
      LynxResourceLoader loader = weakLoader.get();
      if (loader != null) {
        loader.reportError(
            LynxResourceLoader.METHOD_NAME_LOAD_SCRIPT, mUrl, errCode, errMsg, rootCause);
      }
    }
  }
}
