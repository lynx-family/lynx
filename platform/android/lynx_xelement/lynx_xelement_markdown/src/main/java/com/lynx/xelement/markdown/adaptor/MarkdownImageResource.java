// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.text.TextUtils;
import androidx.annotation.Nullable;
import com.facebook.drawee.backends.pipeline.Fresco;
import com.facebook.drawee.controller.BaseControllerListener;
import com.facebook.drawee.drawable.ScalingUtils;
import com.facebook.drawee.generic.GenericDraweeHierarchy;
import com.facebook.drawee.generic.GenericDraweeHierarchyBuilder;
import com.facebook.drawee.interfaces.DraweeController;
import com.facebook.drawee.view.DraweeHolder;
import com.facebook.imagepipeline.common.ResizeOptions;
import com.facebook.imagepipeline.image.ImageInfo;
import com.facebook.imagepipeline.request.ImageRequest;
import com.facebook.imagepipeline.request.ImageRequestBuilder;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.image.ImageUrlRedirectUtils;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class MarkdownImageResource {
  private static final ResizeOptions IMAGE_DOWNSAMPLE_SIZE = new ResizeOptions(1000, 1000);

  private final String mSource;
  private final MarkdownResourceContext mHost;
  private final CountDownLatch mLoadLatch = new CountDownLatch(1);
  private @Nullable DraweeHolder<GenericDraweeHierarchy> mDraweeHolder;
  private @Nullable Drawable mDrawable;

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
    if (mDrawable != null && callback != null) {
      mDrawable.setCallback(callback);
    }
    return mDrawable;
  }

  public void release() {
    UIThreadUtils.runOnUiThreadImmediately(() -> {
      if (mDrawable != null) {
        mDrawable.setCallback(null);
        mDrawable = null;
      }
      if (mDraweeHolder != null) {
        mDraweeHolder.setController(null);
        mDraweeHolder = null;
      }
    });
  }

  private void createDrawable(boolean downSampling) {
    try {
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

      ImageRequestBuilder requestBuilder = ImageRequestBuilder.newBuilderWithSource(uri);
      if (downSampling) {
        requestBuilder.setResizeOptions(IMAGE_DOWNSAMPLE_SIZE);
      }
      ImageRequest request = requestBuilder.build();

      BaseControllerListener<ImageInfo> controllerListener =
          new BaseControllerListener<ImageInfo>() {
            @Override
            public void onFinalImageSet(
                String id, ImageInfo imageInfo, android.graphics.drawable.Animatable animatable) {
              if (mDrawable != null && imageInfo != null && imageInfo.getWidth() > 0
                  && imageInfo.getHeight() > 0) {
                mDrawable.setBounds(0, 0, imageInfo.getWidth(), imageInfo.getHeight());
              }
              mHost.onResourceLoaded();
            }

            @Override
            public void onFailure(String id, Throwable throwable) {
              mHost.onImageLoadError(mSource, throwable);
            }
          };

      mDraweeHolder = new DraweeHolder<>(
          GenericDraweeHierarchyBuilder.newInstance(context.getResources()).build());
      mDraweeHolder.getHierarchy().setActualImageScaleType(ScalingUtils.ScaleType.FIT_CENTER);
      DraweeController controller = Fresco.newDraweeControllerBuilder()
                                        .setAutoPlayAnimations(true)
                                        .setOldController(mDraweeHolder.getController())
                                        .setCallerContext(context.getFrescoCallerContext())
                                        .setImageRequest(request)
                                        .setControllerListener(controllerListener)
                                        .build();
      mDraweeHolder.setController(controller);
      mDrawable = mDraweeHolder.getTopLevelDrawable();
      Drawable.Callback callback = mHost.getDrawableCallback();
      if (mDrawable != null && callback != null) {
        mDrawable.setCallback(callback);
        mDrawable.draw(new Canvas());
      }
    } finally {
      mLoadLatch.countDown();
    }
  }
}
