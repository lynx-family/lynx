// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.provider;

import java.io.InputStream;
import java.util.List;
import java.util.Map;

/**
 * Use @{@link LynxResourceResponse} instead
 */
@Deprecated
public class LynxResResponse {
  private String mMimeType;
  private String mEncoding;
  private int mStatusCode;
  private String mReasonPhrase = "Unknown error occurs in requesting resource.";
  private Map<String, List<String>> mResponseHeaders;
  private InputStream mInputStream;

  public String getMimeType() {
    return mMimeType;
  }

  public void setMimeType(String mimeType) {
    this.mMimeType = mimeType;
  }

  public String getEncoding() {
    return mEncoding;
  }

  public void setEncoding(String encoding) {
    this.mEncoding = encoding;
  }

  public int getStatusCode() {
    return mStatusCode;
  }

  public void setStatusCode(int statusCode) {
    this.mStatusCode = statusCode;
  }

  public String getReasonPhrase() {
    return mReasonPhrase;
  }

  public void setReasonPhrase(String reasonPhrase) {
    this.mReasonPhrase = reasonPhrase;
  }

  public Map<String, List<String>> getResponseHeaders() {
    return mResponseHeaders;
  }

  public void setResponseHeaders(Map<String, List<String>> responseHeaders) {
    this.mResponseHeaders = responseHeaders;
  }

  public InputStream getInputStream() {
    return mInputStream;
  }

  public void setInputStream(InputStream inputStream) {
    this.mInputStream = inputStream;
  }
}
