// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.provider;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringDef;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Map;

/**
 * Use @{@link LynxResourceRequest} instead
 */
@Deprecated
public class LynxResRequest {
  public static final String METHOD_GET = "GET";
  public static final String METHOD_POST = "POST";

  @StringDef({METHOD_GET, METHOD_POST})
  @Retention(RetentionPolicy.SOURCE)
  private @interface ReqMethod {}

  @NonNull private String mUrl;

  @ReqMethod @NonNull private String mMethod;

  @Nullable private Map<String, String> mHeaders;

  @Nullable private String mMimeType;

  @Nullable private String mResponseType;

  @Nullable private String mExtraData;

  @Nullable private Object mLynxExtraData;

  /**
   * @important lynxExtraData must be passed from LynxContext, it corresponds
   * to lynxModuleExtraData in LynxViewBuilder.
   */
  public LynxResRequest(@NonNull String url, @Nullable Object lynxExtraData) {
    this.mUrl = url;
    mMethod = METHOD_GET;
    mLynxExtraData = lynxExtraData;
  }

  @NonNull
  public String getUrl() {
    return mUrl;
  }

  public void setUrl(@NonNull String url) {
    this.mUrl = url;
  }

  @ReqMethod
  public String getMethod() {
    return mMethod;
  }

  public void setMethod(@ReqMethod String method) {
    this.mMethod = method;
  }

  @Nullable
  public Map<String, String> getHeaders() {
    return mHeaders;
  }

  public void setHeaders(@Nullable Map<String, String> header) {
    this.mHeaders = header;
  }

  @Nullable
  public String getMineType() {
    return mMimeType;
  }

  public void setMineType(@Nullable String mimeType) {
    this.mMimeType = mimeType;
  }

  @Nullable
  public String getResponseType() {
    return mResponseType;
  }

  public void setResponseType(@Nullable String responseType) {
    this.mResponseType = responseType;
  }

  @Nullable
  public String getExtraData() {
    return mExtraData;
  }

  public void setExtraData(@Nullable String extraData) {
    this.mExtraData = extraData;
  }

  @Nullable
  public Object getLynxExtraData() {
    return mLynxExtraData;
  }
}
