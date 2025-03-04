// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.provider;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.io.InputStream;

public interface ILynxResourceResponseDataInfo {
  // Whether the resource was successfully obtained
  @NonNull Boolean isSucceed();

  @Nullable byte[] provideBytes();

  @Nullable InputStream provideInputStream();

  @Nullable Object getImage();

  // The path to the resource file
  @Nullable String getFilePath();

  @Nullable String getDataType();

  // Resource source
  @Nullable String getFrom();
}
