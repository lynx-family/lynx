// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core.resource;

import com.lynx.tasm.LynxInfoReportHelper;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.service.security.ILynxSecurityService;
import com.lynx.tasm.service.security.SecurityResult;
import java.nio.ByteBuffer;

/**
 * Provide unified lazy bundle loading callback
 *
 * Template loading of lazy bundles supports three protocols, so provides
 * TemplateCallbackHelper to maintain consistency of behavior of each protocol
 */
class TemplateResourceCallback extends GuardedResourceCallback {
  private final long mResponseHandler;
  private final LynxInfoReportHelper mReportHelper;
  private final LynxResourceRequest.LynxResourceType mResourceType;

  public TemplateResourceCallback(String url, long responseHandler,
      LynxInfoReportHelper reportHelper, LynxResourceRequest.LynxResourceType resourceType) {
    super(url);
    mResponseHandler = responseHandler;
    mReportHelper = reportHelper;
    mResourceType = resourceType;
  }

  public LynxResourceRequest.LynxResourceType getResourceType() {
    return this.mResourceType;
  }

  public void onTemplateLoaded(
      boolean success, byte[] data, TemplateBundle bundle, ByteBuffer buffer, String errorMsg) {
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
        // TODO(zhoupeng.z): add new TASM type for frame
        final ILynxSecurityService.LynxTasmType tasmType =
            mResourceType == LynxResourceRequest.LynxResourceType.LynxResourceTypeTemplate
            ? ILynxSecurityService.LynxTasmType.TYPE_TEMPLATE
            : ILynxSecurityService.LynxTasmType.TYPE_DYNAMIC_COMPONENT;
        SecurityResult result = securityService.verifyTASM(null, data, mUrl, tasmType);
        if (!result.isVerified()) {
          success = false;
          errorMsg = "tasm verify failed, url: " + mUrl;
        }
      }
    }

    LynxResourceLoader.nativeInvokeCallback(mResponseHandler, data,
        bundleValid ? bundle.getNativePtr() : 0L, buffer,
        success ? LynxResourceLoader.RESOURCE_LOADER_SUCCESS
                : LynxResourceLoader.RESOURCE_LOADER_FAILED,
        errorMsg);
  }
}
