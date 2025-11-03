// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.group;

import androidx.annotation.NonNull;

/**
 * Structure that holds the bitmap size cache.
 */
public class BitmapSize {
  private final String source;
  private final int width;
  private final int height;

  public BitmapSize(String source, int width, int height) {
    this.source = source;
    this.width = width;
    this.height = height;
  }

  public String getSource() {
    return this.source;
  }

  public int getWidth() {
    return this.width;
  }

  public int getHeight() {
    return this.height;
  }

  @NonNull
  @Override
  public String toString() {
    return source + ": " + width + " - " + height;
  }
}
