// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core.resource;

import com.lynx.tasm.LynxInfoReportHelper;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.service.security.ILynxSecurityService;
import com.lynx.tasm.service.security.SecurityResult;

/**
 * Provide unified lazy bundle loading callback
 *
 * Template loading of lazy bundles supports three protocols, so provides
 * TemplateCallbackHelper to maintain consistency of behavior of each protocol
 */
class TemplateResourceCallback extends GuardedResourceCallback {
  private final long mResponseHandler;
  private final LynxInfoReportHelper mReportHelper;

  public TemplateResourceCallback(
      String url, long responseHandler, LynxInfoReportHelper reportHelper) {
    super(url);
    mResponseHandler = responseHandler;
    mReportHelper = reportHelper;
  }

  public void onTemplateLoaded(
      boolean success, byte[] data, TemplateBundle bundle, String errorMsg) {
    if (!EnsureInvokedOnce()) {
      return;
    }

    // Report only when loading async component data success
    final boolean dataValid = data != null && data.length > 0;
    final boolean bundleValid = bundle != null && bundle.isValid();
    if (success && (dataValid || bundleValid) && mReportHelper != null) {
      mReportHelper.reportLynxCrashContext(LynxInfoReportHelper.KEY_ASYNC_COMPONENT_URL, mUrl);
    }

    // verify only when data valid.
    if (success && !bundleValid && dataValid) {
      ILynxSecurityService securityService =
          LynxServiceCenter.inst().getService(ILynxSecurityService.class);
      if (securityService != null) {
        SecurityResult result = securityService.verifyTASM(
            null, data, mUrl, ILynxSecurityService.LynxTasmType.TYPE_DYNAMIC_COMPONENT);
        if (!result.isVerified()) {
          success = false;
          errorMsg = "tasm verify failed, url: " + mUrl;
        }
      }
    }

    LynxResourceLoader.nativeInvokeCallback(mResponseHandler, data,
        bundleValid ? bundle.getNativePtr() : 0L,
        success ? LynxResourceLoader.RESOURCE_LOADER_SUCCESS
                : LynxResourceLoader.RESOURCE_LOADER_FAILED,
        errorMsg);
  }
}
