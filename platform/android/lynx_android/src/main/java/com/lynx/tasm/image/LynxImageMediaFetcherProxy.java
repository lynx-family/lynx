// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image;

import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.image.ImageUrlRedirectUtils;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.media.LynxMediaResourceFetcher;
import com.lynx.tasm.resourceprovider.media.OptionalBool;
import com.lynx.tasm.service.ILynxResourceService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.io.Closeable;

public class LynxImageMediaFetcherProxy extends LynxMediaResourceFetcher {
  private LynxContext mContext;
  private LynxMediaResourceFetcher mFetcherDelegate;

  private boolean mAsyncRedirect;

  private ILynxResourceService mLynxResourceService;

  public static final String KEY_WIDTH = "width";

  public static final String KEY_HEIGHT = "height";

  public LynxImageMediaFetcherProxy(LynxContext context) {
    mContext = context;
    mAsyncRedirect = mContext.isAsyncRedirect() && mContext.getAsyncImageInterceptor() != null;
    mFetcherDelegate = mContext.getMediaResourceFetcher();
    mLynxResourceService = LynxServiceCenter.inst().getService(ILynxResourceService.class);
  }

  @Override
  public String shouldRedirectUrl(LynxResourceRequest request) {
    if (mFetcherDelegate != null) {
      return mFetcherDelegate.shouldRedirectUrl(request);
    } else {
      if (mAsyncRedirect) {
        return ImageUrlRedirectUtils.asyncRedirectUrl(mContext, request.getUrl());
      } else {
        return ImageUrlRedirectUtils.redirectUrl(mContext, request.getUrl());
      }
    }
  }

  @Override
  public OptionalBool isLocalResource(String url) {
    if (mFetcherDelegate != null) {
      return mFetcherDelegate.isLocalResource(url);
    } else if (mLynxResourceService != null) {
      return convertToOptionalBool(mLynxResourceService.isLocalResource(url));
    }
    return OptionalBool.UNDEFINED;
  }

  @Override
  public void fetchImage(LynxResourceRequest request, LynxResourceCallback<Closeable> callback) {}

  private OptionalBool convertToOptionalBool(int result) {
    if (result == ILynxResourceService.RESULT_EXCEPTION) {
      return OptionalBool.UNDEFINED;
    } else if (result == ILynxResourceService.RESULT_IS_LOCAL_RESOURCE) {
      return OptionalBool.TRUE;
    } else if (result == ILynxResourceService.RESULT_IS_NOT_LOCAL_RESOURCE) {
      return OptionalBool.FALSE;
    }
    return OptionalBool.UNDEFINED;
  }
}
