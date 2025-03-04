// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge.network;

import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.base.CalledByNative;

public class HttpResponse {
  private int mStatusCode;
  private String mStatusText;
  private String mUrl;
  private JavaOnlyMap mHttpHeaders;
  private byte[] mHttpBody;
  private JavaOnlyMap mCustomInfo;

  public HttpResponse() {
    mStatusCode = 200;
    mStatusText = "OK";
    mUrl = "";
    mHttpHeaders = new JavaOnlyMap();
    mHttpBody = new byte[0];
    mCustomInfo = new JavaOnlyMap();
  }

  @CalledByNative
  public int getStatusCode() {
    return mStatusCode;
  }

  @CalledByNative
  public String getStatusText() {
    return mStatusText;
  }

  @CalledByNative
  public JavaOnlyMap getHttpHeaders() {
    return mHttpHeaders;
  }

  @CalledByNative
  public byte[] getHttpBody() {
    return mHttpBody;
  }

  @CalledByNative
  public String getUrl() {
    return mUrl;
  }

  @CalledByNative
  public JavaOnlyMap getCustomInfo() {
    return mCustomInfo;
  }

  public void setCustomInfo(JavaOnlyMap customInfo) {
    this.mCustomInfo = customInfo;
  }

  public void setHttpBody(byte[] httpBody) {
    this.mHttpBody = httpBody;
  }

  public void setHttpHeaders(JavaOnlyMap httpHeaders) {
    this.mHttpHeaders = httpHeaders;
  }

  public void setUrl(String url) {
    this.mUrl = url;
  }

  public void setStatusText(String statusText) {
    this.mStatusText = statusText;
  }

  public void setStatusCode(int statusCode) {
    this.mStatusCode = statusCode;
  }
}
