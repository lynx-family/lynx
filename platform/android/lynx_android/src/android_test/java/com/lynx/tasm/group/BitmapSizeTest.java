// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import static org.junit.Assert.*;

import org.junit.Test;

public class BitmapSizeTest {
  @Test
  public void getSource() {
    BitmapSize bitmapSize = new BitmapSize("test.png", 10, 10);
    assertEquals(bitmapSize.getSource(), "test.png");
  }

  @Test
  public void getWidth() {
    BitmapSize bitmapSize = new BitmapSize("test.png", 10, 10);
    assertEquals(bitmapSize.getWidth(), 10);
  }

  @Test
  public void getHeight() {
    BitmapSize bitmapSize = new BitmapSize("test.png", 10, 10);
    assertEquals(bitmapSize.getHeight(), 10);
  }

  @Test
  public void testToString() {
    BitmapSize bitmapSize = new BitmapSize("test.png", 10, 10);
    assertEquals(bitmapSize.toString(), "test.png: 10 - 10");
  }
}
