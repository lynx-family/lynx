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
class GenericResourceCallback extends GuardedResourceCallback {
  private final long mResponseHandler;
  private WeakReference<LynxResourceLoader> weakLoader;

  GenericResourceCallback(LynxResourceLoader loader, String url, long responseHandler) {
    super(url);
    this.weakLoader = new WeakReference<>(loader);
    this.mResponseHandler = responseHandler;
  }

  public void onResourceLoaded(boolean success, byte[] data, String errorMsg) {
    if (!EnsureInvokedOnce()) {
      // Ensure responseHandler should be invoked just once.
      return;
    }
    int errCode = LynxResourceLoader.RESOURCE_LOADER_SUCCESS;
    if (success) {
      LLog.i(LynxResourceLoader.TAG, "load resource success with url: " + mUrl);
      if (data == null || data.length == 0) {
        errCode = LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED;
        errorMsg = LynxResourceLoader.MSG_NULL_DATA;
      }
      LynxResourceLoader.InvokeNativeCallbackWithBytes(mResponseHandler, data, errCode, errorMsg);
    } else {
      LLog.i(LynxResourceLoader.TAG,
          "load resource failed with url: " + mUrl + " error message: " + errorMsg);
      errCode = LynxSubErrorCode.E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED;
      LynxResourceLoader.InvokeNativeCallbackWithBytes(
          mResponseHandler, null, errCode, errorMsg + ": " + errorMsg);
    }
  }
}
