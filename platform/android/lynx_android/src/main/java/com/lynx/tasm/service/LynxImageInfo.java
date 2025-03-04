// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import com.lynx.tasm.LynxView;
import java.lang.ref.WeakReference;

/**
 * Data model for reporting lynx image info
 */
public class LynxImageInfo {
  /**
   * image url
   */
  private final String url;

  /**
   * lynx view
   */
  private final WeakReference<LynxView> lynxView;

  /**
   * whether image load is success
   */
  private final boolean isSuccess;

  /**
   * error code
   */
  private final int errorCode;

  /**
   * whether image load hit memory cache
   */
  private final boolean hitMemoryCache;

  /**
   * memory cost
   */
  private final int memoryCost;

  /**
   * time stamp on load start
   */
  private final long startTimeStamp;

  /**
   * time stamp on load finish
   */
  private final long finishTimeStamp;

  public long getStartTimeStamp() {
    return startTimeStamp;
  }

  public long getFinishTimeStamp() {
    return finishTimeStamp;
  }

  public boolean getIsSuccess() {
    return isSuccess;
  }

  public String getUrl() {
    return url;
  }

  public int getMemoryCost() {
    return memoryCost;
  }

  public int getErrorCode() {
    return errorCode;
  }

  public LynxView getLynxView() {
    return lynxView.get();
  }

  public boolean getHitMemoryCache() {
    return hitMemoryCache;
  }

  /**
   * use builder to create LynxImageInfo instance
   */
  private LynxImageInfo(Builder builder) {
    this.startTimeStamp = builder.startTimeStamp;
    this.finishTimeStamp = builder.finishTimeStamp;
    this.isSuccess = builder.isSuccess;
    this.url = builder.url;
    this.memoryCost = builder.memoryCost;
    this.errorCode = builder.errorCode;
    this.lynxView = new WeakReference<>(builder.lynxView);
    this.hitMemoryCache = builder.hitMemoryCache;
  }

  /**
   * build a new LynxImageInfo instance
   */
  public static class Builder {
    private long startTimeStamp = 0;
    private long finishTimeStamp = 0;
    private boolean isSuccess = false;
    private String url = null;
    private int memoryCost = 0;
    private int errorCode = 0;
    private LynxView lynxView = null;
    private boolean hitMemoryCache = false;

    public Builder startTimeStamp(long startTimeStamp) {
      this.startTimeStamp = startTimeStamp;
      return this;
    }
    public Builder finishTimeStamp(long finishTimeStamp) {
      this.finishTimeStamp = finishTimeStamp;
      return this;
    }
    public Builder isSuccess(boolean isSuccess) {
      this.isSuccess = isSuccess;
      return this;
    }
    public Builder url(String url) {
      this.url = url;
      return this;
    }
    public Builder memoryCost(int memoryCost) {
      this.memoryCost = memoryCost;
      return this;
    }
    public Builder errorCode(int errorCode) {
      this.errorCode = errorCode;
      return this;
    }
    public Builder lynxView(LynxView lynxView) {
      this.lynxView = lynxView;
      return this;
    }
    public Builder hitMemoryCache(boolean hitMemoryCache) {
      this.hitMemoryCache = hitMemoryCache;
      return this;
    }

    public LynxImageInfo build() {
      return new LynxImageInfo(this);
    }
  }
}
