// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax.util;

import static com.lynx.animax.util.StreamUtil.getByteArrayFromInputStream;
import static com.lynx.animax.util.StreamUtil.saveFileFromInputStream;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.provider.LynxResResponse;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;
import java.util.Map;

public class LynxResourceUtil {
  private static final String TAG = "LynxResourceUtil";

  public static int getLynxResResponseContentLength(@NonNull LynxResResponse lynxResResponse) {
    Map<String, List<String>> header = lynxResResponse.getResponseHeaders();
    if (header != null) {
      List<String> valueList = header.get("content-length");
      if (valueList != null && !valueList.isEmpty()) {
        return Integer.parseInt(valueList.get(0));
      }
    }
    return 0;
  }

  public static int getTotalLengthForLynxResResponse(@NonNull LynxResResponse response) {
    InputStream stream = response.getInputStream();
    if (stream == null) {
      return 0;
    }
    int totalLength = 0;
    try {
      totalLength = stream.available();
    } catch (IOException ignored) {
    }
    int responseContentLength = getLynxResResponseContentLength(response);
    if (totalLength <= 0) {
      // The available method of InputStream returns the number of bytes that can be read by this
      // input stream without blocking. For nonblocking stream like FileStream, stream.available()
      // returns the file length. For network socket stream, stream.available() returns zero
      // before the first read(), and will not be accurate until the subsequent download is
      // complete.
      AnimaXLog.i(TAG, "no length from stream, responseContentLength = " + responseContentLength);
      if (responseContentLength > 0) {
        totalLength = responseContentLength;
      }
    }
    return totalLength;
  }

  public static boolean saveFileFromLynxResResponse(
      @NonNull LynxResResponse response, String dstFilePath) {
    InputStream inputStream = response.getInputStream();
    int inputStreamLengthHint = getTotalLengthForLynxResResponse(response);
    return saveFileFromInputStream(inputStream, inputStreamLengthHint, dstFilePath);
  }

  public static @Nullable byte[] getByteArrayFromLynxResResponse(
      @NonNull LynxResResponse response) {
    InputStream inputStream = response.getInputStream();
    int inputStreamLengthHint = getTotalLengthForLynxResResponse(response);
    return getByteArrayFromInputStream(inputStream, inputStreamLengthHint);
  }
}
