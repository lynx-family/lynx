// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import static org.junit.Assert.*;

import java.util.Random;
import org.junit.Test;

public class ScreencastDiffTest {
  @Test
  public void firstFrameCoversWholeImage() {
    ScreencastDiff.Region region = ScreencastDiff.compare(null, new int[123 * 65], 123, 65);
    assertEquals(0, region.x);
    assertEquals(0, region.y);
    assertEquals(123, region.width);
    assertEquals(65, region.height);
    assertFalse(region.isSmall(123, 65));
  }

  @Test
  public void identicalPixelsSkipEncoding() {
    assertNull(ScreencastDiff.compare(new int[800 * 600], new int[800 * 600], 800, 600));
  }

  @Test
  public void smallChangeIncludesJpegPadding() {
    int[] previous = new int[512 * 512];
    int[] current = previous.clone();
    current[130 * 512 + 130] = 1;
    ScreencastDiff.Region region = ScreencastDiff.compare(previous, current, 512, 512);
    assertEquals(112, region.x);
    assertEquals(112, region.y);
    assertEquals(96, region.width);
    assertEquals(96, region.height);
    assertTrue(region.isSmall(512, 512));
  }

  @Test
  public void lastPixelOfPartialTileIsNotLost() {
    int[] previous = new int[131 * 67];
    int[] current = previous.clone();
    current[current.length - 1] = 7;
    ScreencastDiff.Region region = ScreencastDiff.compare(previous, current, 131, 67);
    assertEquals(131, region.x + region.width);
    assertEquals(67, region.y + region.height);
    assertEquals(112, region.x);
    assertEquals(48, region.y);
  }

  @Test
  public void separatedChangesFallBackToFullFrame() {
    int[] previous = new int[512 * 512];
    int[] current = previous.clone();
    current[0] = 1;
    current[current.length - 1] = 1;
    assertFalse(ScreencastDiff.compare(previous, current, 512, 512).isSmall(512, 512));
  }

  @Test
  public void thresholdIncludesExactlyThirtyFivePercent() {
    assertFalse(new ScreencastDiff.Region(0, 0, 35, 100).isSmall(100, 100));
    assertTrue(new ScreencastDiff.Region(0, 0, 34, 100).isSmall(100, 100));
  }

  @Test
  public void replacingDirtyRegionReconstructsAllChangedPixels() {
    Random random = new Random(420);
    for (int trial = 0; trial < 100; trial++) {
      int width = 1 + random.nextInt(800);
      int height = 1 + random.nextInt(500);
      int[] previous = new int[width * height];
      for (int i = 0; i < previous.length; i++) {
        previous[i] = random.nextInt();
      }
      int[] current = previous.clone();
      for (int i = 0; i < 20; i++) {
        current[random.nextInt(current.length)] ^= 1;
      }
      ScreencastDiff.Region region = ScreencastDiff.compare(previous, current, width, height);
      if (region != null) {
        for (int y = region.y; y < region.y + region.height; y++) {
          System.arraycopy(
              current, y * width + region.x, previous, y * width + region.x, region.width);
        }
      }
      assertArrayEquals(current, previous);
    }
  }
}
