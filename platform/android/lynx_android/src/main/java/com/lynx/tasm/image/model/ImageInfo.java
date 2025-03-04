// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image.model;
public class ImageInfo {
  private final int mWidth;
  private final int mHeight;
  private final boolean mIsAnim;

  public ImageInfo(int width, int height, boolean isAnim) {
    this.mWidth = width;
    this.mHeight = height;
    this.mIsAnim = isAnim;
  }

  public int getWidth() {
    return mWidth;
  }

  public int getHeight() {
    return mHeight;
  }

  public boolean isAnim() {
    return mIsAnim;
  }
}
