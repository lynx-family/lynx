// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image;

import androidx.annotation.Nullable;
import com.lynx.tasm.LynxViewClientV2;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;

public class LynxImageLoadInfo extends LynxViewClientV2.LynxResourceLoadInfo {
  private String mSrc;
  private int mWidth;
  private int mHeight;
  private int mViewWidth;
  private int mViewHeight;
  private long mLoadStart;
  private long mLoadFinish;
  private int mOrigin;

  /**
   * Create a new LynxResourceLoadInfo.
   *
   * @param type the resource type
   * @param errCode the error code of the load result, 0 means success
   * @param errMsg  the error message of the load result, if any
   */
  public LynxImageLoadInfo(
      LynxResourceRequest.LynxResourceType type, int errCode, @Nullable String errMsg) {
    super(type, errCode, errMsg);
  }

  public LynxImageLoadInfo setSrc(String src) {
    this.mSrc = src;
    return this;
  }

  public String getSrc() {
    return mSrc;
  }

  public LynxImageLoadInfo setWidth(int width) {
    this.mWidth = width;
    return this;
  }

  public int getWidth() {
    return mWidth;
  }

  public LynxImageLoadInfo setHeight(int height) {
    this.mHeight = height;
    return this;
  }

  public int getHeight() {
    return mHeight;
  }

  public LynxImageLoadInfo setViewWidth(int viewWidth) {
    this.mViewWidth = viewWidth;
    return this;
  }

  public int getViewWidth() {
    return mViewWidth;
  }

  public LynxImageLoadInfo setViewHeight(int viewHeight) {
    this.mViewHeight = viewHeight;
    return this;
  }

  public int getViewHeight() {
    return mViewHeight;
  }

  public LynxImageLoadInfo setLoadStart(long loadStart) {
    this.mLoadStart = loadStart;
    return this;
  }

  public long getLoadStart() {
    return mLoadStart;
  }

  public LynxImageLoadInfo setLoadFinish(long loadFinish) {
    this.mLoadFinish = loadFinish;
    return this;
  }

  public long getLoadFinish() {
    return mLoadFinish;
  }

  public LynxImageLoadInfo setOrigin(int origin) {
    this.mOrigin = origin;
    return this;
  }

  public int getOrigin() {
    return mOrigin;
  }
}
