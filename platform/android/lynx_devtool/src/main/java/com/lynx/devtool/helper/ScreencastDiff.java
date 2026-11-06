// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

/** Exact pixel comparison before compression. Coordinates are in the encoded image. */
final class ScreencastDiff {
  private static final int TILE_SIZE = 64;
  private static final int JPEG_BLOCK = 16;

  static final class Region {
    final int x;
    final int y;
    final int width;
    final int height;

    Region(int x, int y, int width, int height) {
      this.x = x;
      this.y = y;
      this.width = width;
      this.height = height;
    }

    boolean isSmall(int fullWidth, int fullHeight) {
      return (long) width * height * 100 < (long) fullWidth * fullHeight * 35;
    }
  }

  static Region compare(int[] previous, int[] current, int width, int height) {
    if (width <= 0 || height <= 0 || (long) width * height > current.length) {
      throw new IllegalArgumentException("Invalid screencast dimensions");
    }
    if (previous == null || previous.length != current.length) {
      return new Region(0, 0, width, height);
    }
    int left = width;
    int top = height;
    int right = 0;
    int bottom = 0;
    for (int y = 0; y < height; y += TILE_SIZE) {
      for (int x = 0; x < width; x += TILE_SIZE) {
        int endX = Math.min(width, x + TILE_SIZE);
        int endY = Math.min(height, y + TILE_SIZE);
        boolean changed = false;
        for (int row = y; row < endY && !changed; row++) {
          for (int column = x; column < endX; column++) {
            int index = row * width + column;
            if (previous[index] != current[index]) {
              changed = true;
              break;
            }
          }
        }
        if (changed) {
          left = Math.min(left, x);
          top = Math.min(top, y);
          right = Math.max(right, endX);
          bottom = Math.max(bottom, endY);
        }
      }
    }
    if (right == 0) {
      return null;
    }
    // Include a JPEG MCU around the changed tiles to avoid visible seams at patch edges.
    left = Math.max(0, left - JPEG_BLOCK);
    top = Math.max(0, top - JPEG_BLOCK);
    right = Math.min(width, right + JPEG_BLOCK);
    bottom = Math.min(height, bottom + JPEG_BLOCK);
    return new Region(left, top, right - left, bottom - top);
  }

  private ScreencastDiff() {}
}
