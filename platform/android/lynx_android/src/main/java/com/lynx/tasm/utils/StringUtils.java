// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import androidx.annotation.Nullable;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class StringUtils {
  @Nullable
  public static String streamToString(InputStream stream) {
    byte[] bytes = streamToBytes(stream);
    return bytes == null ? null : new String(bytes);
  }

  @Nullable
  public static byte[] streamToBytes(InputStream is) {
    if (null == is) {
      return null;
    }
    ByteArrayOutputStream buffer = new ByteArrayOutputStream();
    int nRead;
    byte[] data = new byte[2048];
    try {
      while ((nRead = is.read(data, 0, data.length)) != -1) {
        buffer.write(data, 0, nRead);
      }
      return buffer.toByteArray();
    } catch (IOException e) {
      return null;
    }
  }
}
