// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.provider.ILynxResourceResponseDataInfo;
import java.io.File;
import java.io.InputStream;
import java.util.Map;

public interface ILynxResourceServiceResponse extends ILynxResourceResponseDataInfo {
  // Whether the resource was successfully obtained
  @NonNull Boolean isSucceed();

  // The path to the resource file
  @Nullable String getFilePath();

  // resource source
  @Nullable String getFrom();

  // The original resource source of the memory cache
  @NonNull String getOriginFrom();

  // Is it from cache
  @NonNull Boolean isCache();

  @NonNull Long getVersion();

  // The loader that finally gets the resource
  @NonNull String getSuccessFetcher();

  // Is it cancelled
  @NonNull Boolean isCanceled();

  @NonNull Boolean isPreloaded();

  @NonNull Boolean isRequestReused();

  // Performance-related time-consuming information
  @NonNull Map<String, Long> getPerformanceInfo();

  @Nullable InputStream provideInputStream();

  @Nullable byte[] provideBytes();

  @Nullable File provideFile();

  @Nullable String getDataType();

  @NonNull String getSourceType();

  @NonNull String getErrorInfoString();

  @NonNull Integer getErrorCode();

  @Nullable String getCharset();

  @Nullable Object getImage();

  @NonNull Boolean getHasBeenPaused();

  @NonNull Boolean getIsDataTypeEmpty();
}
