// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.io.File;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;

class LynxResourceServiceProxyErrorResponse implements ILynxResourceServiceResponse {
  private Integer mErrorCode;
  private String mErrorMessage;

  LynxResourceServiceProxyErrorResponse(Integer errorCode, String errorMessage) {
    this.mErrorCode = errorCode;
    this.mErrorMessage = errorMessage;
  }

  @NonNull
  @Override
  public Boolean isSucceed() {
    return false;
  }

  @Nullable
  @Override
  public String getFilePath() {
    return null;
  }

  @Nullable
  @Override
  public String getFrom() {
    return null;
  }

  @NonNull
  @Override
  public String getOriginFrom() {
    return "";
  }

  @NonNull
  @Override
  public Boolean isCache() {
    return false;
  }

  @NonNull
  @Override
  public Long getVersion() {
    return 0L;
  }

  @NonNull
  @Override
  public String getSuccessFetcher() {
    return "";
  }

  @NonNull
  @Override
  public Boolean isCanceled() {
    return true;
  }

  @NonNull
  @Override
  public Boolean isPreloaded() {
    return false;
  }

  @NonNull
  @Override
  public Boolean isRequestReused() {
    return false;
  }

  @NonNull
  @Override
  public Map<String, Long> getPerformanceInfo() {
    return new HashMap<>();
  }

  @Nullable
  @Override
  public InputStream provideInputStream() {
    return null;
  }

  @Nullable
  @Override
  public byte[] provideBytes() {
    return null;
  }

  @Nullable
  @Override
  public File provideFile() {
    return null;
  }

  @Nullable
  @Override
  public String getDataType() {
    return null;
  }

  @NonNull
  @Override
  public String getSourceType() {
    return "";
  }

  @NonNull
  @Override
  public String getErrorInfoString() {
    return this.mErrorMessage;
  }

  @NonNull
  @Override
  public Integer getErrorCode() {
    return this.mErrorCode;
  }

  @Nullable
  @Override
  public String getCharset() {
    return null;
  }

  @Nullable
  @Override
  public Object getImage() {
    return null;
  }

  @NonNull
  @Override
  public Boolean getHasBeenPaused() {
    return false;
  }

  @NonNull
  @Override
  public Boolean getIsDataTypeEmpty() {
    return true;
  }
}
