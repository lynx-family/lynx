// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image.model;

import android.graphics.Bitmap;
import java.util.List;
import java.util.Map;

public class ImageRequestInfoBuilder {
  private String mUrl;

  private int mResizeWidth;

  private int mResizeHeight;

  private int mLoopCount;

  private Bitmap.Config mConfig;

  private boolean mEnableGifLiteDecoder;

  private Map<String, String> mCustomParam;

  private @CacheChoice int mCacheChoice;

  private @DiskCacheChoice int mDiskCacheChoice;

  private List<BitmapPostProcessor> mProcessors;

  private boolean mEnableAsyncRequest;

  private boolean mEnableAnimationAutoPlay;

  private boolean mEnableDownSampling = true;

  private boolean mEnableResourceHint;

  private Object mCallerContext;

  private boolean mUseLocalCache;

  public static ImageRequestInfoBuilder newBuilderWithSource(String url) {
    return new ImageRequestInfoBuilder().setUrl(url);
  }

  public boolean isEnableDownSampling() {
    return mEnableDownSampling;
  }

  public ImageRequestInfoBuilder setEnableDownSampling(boolean mEnableDownSampling) {
    this.mEnableDownSampling = mEnableDownSampling;
    return this;
  }

  public boolean isEnableAnimationAutoPlay() {
    return mEnableAnimationAutoPlay;
  }

  public ImageRequestInfoBuilder setEnableAnimationAutoPlay(boolean mEnableAnimationAutoPlay) {
    this.mEnableAnimationAutoPlay = mEnableAnimationAutoPlay;
    return this;
  }

  public ImageRequestInfoBuilder setUrl(String url) {
    mUrl = url;
    return this;
  }

  public ImageRequestInfoBuilder setResizeWidth(int width) {
    mResizeWidth = width;
    return this;
  }

  public ImageRequestInfoBuilder setResizeHeight(int height) {
    mResizeHeight = height;
    return this;
  }

  public void setCustomParam(Map<String, String> customParam) {
    this.mCustomParam = customParam;
  }

  public ImageRequestInfoBuilder setLoopCount(int loopCount) {
    mLoopCount = loopCount;
    return this;
  }

  public ImageRequestInfoBuilder setBitmapConfig(Bitmap.Config config) {
    mConfig = config;
    return this;
  }

  public ImageRequestInfoBuilder setEnableGifLiteDecoder(boolean enableGifLiteDecoder) {
    mEnableGifLiteDecoder = enableGifLiteDecoder;
    return this;
  }

  public ImageRequestInfoBuilder setCacheChoice(@CacheChoice int cacheChoice) {
    mCacheChoice = cacheChoice;
    return this;
  }

  public ImageRequestInfoBuilder setBitmapPostProcessor(List<BitmapPostProcessor> postProcessors) {
    this.mProcessors = postProcessors;
    return this;
  }

  public ImageRequestInfoBuilder setEnableResourceHint(boolean enable) {
    this.mEnableResourceHint = enable;
    return this;
  }

  public ImageRequestInfoBuilder setUseLocalCache(boolean enable) {
    this.mUseLocalCache = enable;
    return this;
  }

  public Object getCallerContext() {
    return mCallerContext;
  }

  public ImageRequestInfoBuilder setCallerContext(Object callerContext) {
    this.mCallerContext = callerContext;
    return this;
  }

  public @DiskCacheChoice int getDiskCacheChoice() {
    return mDiskCacheChoice;
  }

  public void setDiskCacheChoice(@DiskCacheChoice int diskCacheChoice) {
    this.mDiskCacheChoice = diskCacheChoice;
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

  public boolean enableGifLiteDecoder() {
    return mEnableGifLiteDecoder;
  }

  public Map<String, String> getCustomParam() {
    return mCustomParam;
  }

  public boolean isEnableAsyncRequest() {
    return mEnableAsyncRequest;
  }

  public boolean isEnableResourceHint() {
    return mEnableResourceHint;
  }

  public boolean isUseLocalCache() {
    return mUseLocalCache;
  }

  public ImageRequestInfoBuilder setEnableAsyncRequest(boolean mEnableAsyncRequest) {
    this.mEnableAsyncRequest = mEnableAsyncRequest;
    return this;
  }

  public @CacheChoice int getCacheChoice() {
    return mCacheChoice;
  }

  public List<BitmapPostProcessor> getProcessors() {
    return mProcessors;
  }

  public ImageRequestInfo build() {
    return new ImageRequestInfo(this);
  }
}
