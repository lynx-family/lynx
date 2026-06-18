// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image.model;

public class ImageInfo {
  private static final int IMAGE_ORIGIN_UNKNOWN = -1;

  private final int mWidth;
  private final int mHeight;
  private final boolean mIsAnim;
  private final int mOrigin;

  public ImageInfo(int width, int height, boolean isAnim) {
    this(width, height, isAnim, IMAGE_ORIGIN_UNKNOWN);
  }

  public ImageInfo(int width, int height, boolean isAnim, int origin) {
    mWidth = width;
    mHeight = height;
    mIsAnim = isAnim;
    mOrigin = origin;
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

  public int getOrigin() {
    return mOrigin;
  }
}
