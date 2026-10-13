// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.text.TextUtils;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.image.ImageUrlRedirectUtils;
import com.lynx.tasm.image.ImageContent;
import com.lynx.tasm.image.LynxScaleTypeDrawable;
import com.lynx.tasm.image.ScalingUtils;
import com.lynx.tasm.image.model.ImageInfo;
import com.lynx.tasm.image.model.ImageLoadListener;
import com.lynx.tasm.image.model.ImageRequestInfo;
import com.lynx.tasm.image.model.ImageRequestInfoBuilder;
import com.lynx.tasm.service.ILynxImageService;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import org.json.JSONObject;

public class MarkdownImageResource {
  private static final int IMAGE_DOWNSAMPLE_SIZE = 1000;

  private final String mSource;
  private final MarkdownResourceContext mHost;
  private final CountDownLatch mLoadLatch = new CountDownLatch(1);
  private @Nullable Drawable mDrawable;
  private @Nullable Drawable.Callback mDrawableCallback;
  private @Nullable ILynxImageService mImageService;
  private @Nullable ImageRequestInfo mImageRequestInfo;
  private @Nullable ImageContent mImageContent;
  private boolean mReleased;

  public MarkdownImageResource(
      String source, boolean downSampling, boolean syncLoading, MarkdownResourceContext host) {
    mSource = source;
    mHost = host;
    UIThreadUtils.runOnUiThreadImmediately(() -> createDrawable(downSampling));
    if (syncLoading) {
      try {
        mLoadLatch.await(100, TimeUnit.MILLISECONDS);
      } catch (InterruptedException ignored) {
      }
    }
  }

  @Nullable
  public Drawable getDrawable(@Nullable Drawable.Callback callback) {
    mDrawableCallback = callback;
    if (mDrawable != null) {
      mDrawable.setCallback(callback);
    }
    if (mImageContent != null) {
      mImageContent.setCallback(callback);
    }
    return mDrawable;
  }

  public void release() {
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      if (mReleased) {
        return;
      }
      mReleased = true;
      mDrawableCallback = null;
      if (mImageContent != null) {
        releaseImageContent(mImageContent);
        mImageContent = null;
      }
      if (mImageRequestInfo != null) {
        mImageService.releaseImage(mImageRequestInfo);
        mImageRequestInfo = null;
      }
      if (mDrawable != null) {
        mDrawable.setCallback(null);
        mDrawable = null;
      }
    });
  }

  private void createDrawable(boolean downSampling) {
    try {
      if (mReleased) {
        return;
      }
      LynxContext context = mHost.getLynxContext();
      if (context == null) {
        return;
      }
      String source = ImageUrlRedirectUtils.redirectUrl(context, mSource);
      if (TextUtils.isEmpty(source)) {
        mHost.onImageLoadError(mSource, null);
        return;
      }
      Uri uri = Uri.parse(source);
      if (uri == null || uri == Uri.EMPTY || uri.getScheme() == null) {
        mHost.onImageLoadError(mSource, null);
        return;
      }

      mImageService = LynxServiceCenter.inst().getService(ILynxImageService.class);
      if (mImageService == null) {
        mHost.onImageLoadError(
            mSource, new IllegalStateException("Lynx image service is unavailable."));
        return;
      }
      fetchImage(source, downSampling, context);
    } finally {
      mLoadLatch.countDown();
    }
  }

  private void fetchImage(String source, boolean downSampling, LynxContext context) {
    ImageRequestInfoBuilder builder = ImageRequestInfoBuilder.newBuilderWithSource(source)
                                          .setCallerContext(context.getFrescoCallerContext())
                                          .setEnableDownSampling(downSampling)
                                          .setEnableAnimationAutoPlay(true);
    if (downSampling) {
      builder.setResizeWidth(IMAGE_DOWNSAMPLE_SIZE).setResizeHeight(IMAGE_DOWNSAMPLE_SIZE);
    }
    mImageRequestInfo = builder.build();
    mImageService.fetchImage(mImageRequestInfo, new ImageLoadListener() {
      @Override
      public void onRequestSubmit(ImageRequestInfo requestInfo) {}

      @Override
      public void onSuccess(
          @Nullable ImageContent imageContent, ImageRequestInfo requestInfo, ImageInfo imageInfo) {
        UIThreadUtils.runOnUiThreadImmediately(() -> {
          if (mReleased) {
            if (imageContent != null) {
              releaseImageContent(imageContent);
            }
            return;
          }
          if (imageContent == null) {
            mHost.onImageLoadError(mSource, null);
            return;
          }
          mImageContent = imageContent;
          mDrawable = new LynxScaleTypeDrawable(imageContent, ScalingUtils.ScaleType.FIT_CENTER);
          float density = context.getScreenMetrics().density;
          mDrawable.setBounds(0, 0, (int) (imageInfo.getWidth() * density),
              (int) (imageInfo.getHeight() * density));
          mDrawable.setCallback(mDrawableCallback);
          imageContent.setCallback(mDrawableCallback);
          mHost.onImageLoaded(mSource);
        });
      }

      @Override
      public void onFailure(int errorCode, Throwable throwable) {
        UIThreadUtils.runOnUiThreadImmediately(() -> {
          if (!mReleased) {
            mHost.onImageLoadError(mSource, throwable);
          }
        });
      }

      @Override
      public void onImageMonitorInfo(JSONObject monitorInfo) {}
    }, null, context);
  }

  private void releaseImageContent(ImageContent imageContent) {
    Drawable drawable = imageContent.getDrawable();
    if (drawable != null) {
      drawable.setCallback(null);
      mImageService.stopAnimation(drawable);
      mImageService.releaseAnimDrawable(drawable);
    }
    imageContent.releaseImageResource();
  }
}
