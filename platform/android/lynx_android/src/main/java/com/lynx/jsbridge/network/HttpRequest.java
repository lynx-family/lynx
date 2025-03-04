// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge.network;

import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.base.CalledByNative;

public class HttpRequest {
  private String mHttpMethod;
  private String mUrl;
  private String mOriginUrl;
  private byte[] mHttpBody;
  private JavaOnlyMap mHttpHeaders;
  private JavaOnlyMap mCustomConfig;

  public HttpRequest() {
    this.mHttpMethod = "";
    this.mUrl = "";
    this.mOriginUrl = "";
    this.mHttpBody = null;
    this.mHttpHeaders = new JavaOnlyMap();
    this.mCustomConfig = new JavaOnlyMap();
  }

  @CalledByNative
  public static HttpRequest CreateHttpRequest(String httpMethod, String url, String originUrl,
      byte[] httpBody, JavaOnlyMap httpHeaderFields, JavaOnlyMap customConfig) {
    HttpRequest request = new HttpRequest();
    request.mHttpMethod = httpMethod;
    request.mUrl = url;
    request.mOriginUrl = originUrl;
    request.mHttpBody = httpBody;
    request.mHttpHeaders = httpHeaderFields;
    request.mCustomConfig = customConfig;
    return request;
  }

  public String getHttpMethod() {
    return mHttpMethod;
  }

  public void setHttpMethod(String httpMethod) {
    this.mHttpMethod = httpMethod;
  }

  public String getUrl() {
    return mUrl;
  }

  public void setUrl(String url) {
    this.mUrl = url;
  }

  public String getOriginUrl() {
    return mOriginUrl;
  }

  public void setOriginUrl(String originUrl) {
    this.mOriginUrl = originUrl;
  }

  public byte[] getHttpBody() {
    return mHttpBody;
  }

  public void setHttpBody(byte[] httpBody) {
    this.mHttpBody = httpBody;
  }

  public JavaOnlyMap getHttpHeaders() {
    return mHttpHeaders;
  }

  public void setHttpHeaders(JavaOnlyMap httpHeaders) {
    this.mHttpHeaders = httpHeaders;
  }

  public JavaOnlyMap getCustomConfig() {
    return mCustomConfig;
  }

  public void setCustomConfig(JavaOnlyMap customConfig) {
    this.mCustomConfig = customConfig;
  }
}
