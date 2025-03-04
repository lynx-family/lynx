// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.util.Base64;
import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

public class TestbenchFileUtil {
  private static final int READ_BUFFER = 1024 * 4;

  public static String getBase64StringFile(String filePath) {
    InputStream inputStream;
    String encodedStr = "";
    ByteArrayOutputStream output = new ByteArrayOutputStream();
    try {
      inputStream = new FileInputStream(filePath);
      byte[] buffer = new byte[READ_BUFFER]; // specify the size to allow
      int bytesRead;
      while ((bytesRead = inputStream.read(buffer)) != -1) {
        output.write(buffer, 0, bytesRead);
      }
    } catch (FileNotFoundException e1) {
      e1.printStackTrace();
      return encodedStr;
    } catch (IOException e) {
      e.printStackTrace();
      return encodedStr;
    }
    encodedStr = Base64.encodeToString(output.toByteArray(), Base64.DEFAULT);
    return encodedStr;
  }
}
