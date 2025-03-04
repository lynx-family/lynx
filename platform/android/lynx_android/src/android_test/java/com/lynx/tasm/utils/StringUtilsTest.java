// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static org.junit.Assert.*;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import org.junit.Test;

public class StringUtilsTest {
  public static final String INPUT_STRING = "Hello Lynx";

  @Test
  public void streamToString() {
    byte[] bytes = INPUT_STRING.getBytes(StandardCharsets.UTF_8);
    InputStream in = null;
    try {
      in = new ByteArrayInputStream(bytes);
      assertEquals(INPUT_STRING, StringUtils.streamToString(in));
    } finally {
      if (in != null) {
        try {
          in.close();
        } catch (IOException e) {
          e.printStackTrace();
        }
      }
    }
  }

  @Test
  public void streamToBytes() {
    byte[] bytes = INPUT_STRING.getBytes(StandardCharsets.UTF_8);
    InputStream in = null;
    try {
      in = new ByteArrayInputStream(bytes);
      assertArrayEquals(bytes, StringUtils.streamToBytes(in));
    } finally {
      if (in != null) {
        try {
          in.close();
        } catch (IOException e) {
          e.printStackTrace();
        }
      }
    }
  }
}
