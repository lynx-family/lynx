// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image.model;

import android.graphics.Bitmap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

public class ImageRequestInfo {
  private final String mUrl;

  private final int mResizeWidth;

  private final int mResizeHeight;

  private final int mLoopCount;

  private final Bitmap.Config mConfig;

  private final boolean mEnableResourceHint;

  // TODO(chengjunnan) Perhaps better abstraction is needed here
  private final boolean mEnableGifLiteDecoder;

  private boolean mEnableDownSampling = true;

  private final Map<String, String> mCustomParam;

  private final @CacheChoice int mCacheChoice;

  private final @DiskCacheChoice int mDiskCacheChoice;

  private final boolean mEnableAsyncRequest;

  private final List<BitmapPostProcessor> mProcessors;
  private Object mCallerContext;

  private boolean mAutoPlay;

  private boolean mUseLocalCache;

  ImageRequestInfo(ImageRequestInfoBuilder builder) {
    mUrl = builder.getUrl();
    mResizeWidth = builder.getResizeWidth();
    mEnableAsyncRequest = builder.isEnableAsyncRequest();
    mDiskCacheChoice = builder.getDiskCacheChoice();
    mResizeHeight = builder.getResizeHeight();
    mLoopCount = builder.getLoopCount();
    mConfig = builder.getConfig();
    mEnableGifLiteDecoder = builder.enableGifLiteDecoder();
    mCustomParam = builder.getCustomParam();
    mCacheChoice = builder.getCacheChoice();
    mProcessors = builder.getProcessors();
    mEnableResourceHint = builder.isEnableResourceHint();
    mEnableDownSampling = builder.isEnableDownSampling();
    mCallerContext = builder.getCallerContext();
    mAutoPlay = builder.isEnableAnimationAutoPlay();
    mUseLocalCache = builder.isUseLocalCache();
  }

  public String getUrl() {
    return mUrl;
  }

  public int getResizeWidth() {
    return mResizeWidth;
  }

  public int getResizeHeight() {
    return mResizeHeight;
  }

  public int getLoopCount() {
    return mLoopCount;
  }

  public Bitmap.Config getConfig() {
    return mConfig;
  }

  public boolean isEnableGifLiteDecoder() {
    return mEnableGifLiteDecoder;
  }

  public Map<String, String> getCustomParam() {
    return mCustomParam;
  }

  public @CacheChoice int getCacheChoice() {
    return mCacheChoice;
  }

  public List<BitmapPostProcessor> getProcessors() {
    return mProcessors;
  }

  public boolean isEnableResourceHint() {
    return mEnableResourceHint;
  }

  public boolean isEnableDownSampling() {
    return mEnableDownSampling;
  }

  public boolean isUseLocalCache() {
    return mUseLocalCache;
  }

  public @DiskCacheChoice int getDiskCacheChoice() {
    return mDiskCacheChoice;
  }

  public boolean isEnableAsyncRequest() {
    return mEnableAsyncRequest;
  }

  public Object getCallerContext() {
    return mCallerContext;
  }

  public boolean isAutoPlay() {
    return mAutoPlay;
  }

  @Override
  public boolean equals(Object o) {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;

    ImageRequestInfo that = (ImageRequestInfo) o;

    if (mResizeWidth != that.mResizeWidth)
      return false;
    if (mResizeHeight != that.mResizeHeight)
      return false;
    if (mEnableResourceHint != that.mEnableResourceHint)
      return false;
    if (mEnableGifLiteDecoder != that.mEnableGifLiteDecoder)
      return false;
    if (mEnableDownSampling != that.mEnableDownSampling)
      return false;
    if (mDiskCacheChoice != that.mDiskCacheChoice)
      return false;
    if (mEnableAsyncRequest != that.mEnableAsyncRequest)
      return false;
    if (!Objects.equals(mUrl, that.mUrl))
      return false;
    if (mConfig != that.mConfig)
      return false;
    return Objects.equals(mProcessors, that.mProcessors);
  }

  @Override
  public int hashCode() {
    int result = mUrl != null ? mUrl.hashCode() : 0;
    result = 31 * result + mResizeWidth;
    result = 31 * result + mResizeHeight;
    result = 31 * result + (mConfig != null ? mConfig.hashCode() : 0);
    result = 31 * result + (mEnableResourceHint ? 1 : 0);
    result = 31 * result + (mEnableGifLiteDecoder ? 1 : 0);
    result = 31 * result + (mEnableDownSampling ? 1 : 0);
    result = 31 * result + mDiskCacheChoice;
    result = 31 * result + (mEnableAsyncRequest ? 1 : 0);
    result = 31 * result + (mProcessors != null ? mProcessors.hashCode() : 0);
    return result;
  }
}
