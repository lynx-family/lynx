// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.provider;

import androidx.annotation.Nullable;
import com.lynx.tasm.service.LynxResourceServiceRequestParams;

/**
 * Standard Lynx Resource Request
 * T: generic type for request, default Object
 */
public class LynxResourceRequest<T> {
  public enum LynxResourceType {
    LynxResourceTypeImage,
    LynxResourceTypeFont,
    LynxResourceTypeLottie,
    LynxResourceTypeVideo,
    LynxResourceTypeSVG,
    LynxResourceTypeTemplate,
    LynxResourceTypeLynxCoreJS,
    LynxResourceTypeDynamicComponent,
    LynxResourceTypeI18NText,
    LynxResourceTypeTheme,
    LynxResourceTypeExternalJSSource
  }

  private String mUrl;
  private T mRequestParams; // for external request params object, default Object type
  private LynxResourceType mRequestResourceType; // for generic lynx resource provider

  public LynxResourceRequest(String url) {
    mUrl = url;
  }

  public LynxResourceRequest(String url, T requestParams) {
    mUrl = url;
    this.mRequestParams = requestParams;
  }

  public LynxResourceRequest(String url, T requestParams, LynxResourceType resourceType) {
    this.mUrl = url;
    this.mRequestParams = requestParams;
    this.mRequestResourceType = resourceType;
  }

  public String getUrl() {
    return mUrl;
  }

  public T getRequestParams() {
    return mRequestParams;
  }

  // Only for LynxResourceFetcher use. Return the full request parameters for LynxResourceService
  @Nullable
  public LynxResourceServiceRequestParams getLynxResourceServiceRequestParams() {
    if (mRequestParams != null && mRequestParams instanceof LynxResourceServiceRequestParams) {
      return (LynxResourceServiceRequestParams) mRequestParams;
    }
    return null;
  }

  @Nullable
  public LynxResourceType getRequestResourceType() {
    return mRequestResourceType;
  }
}
