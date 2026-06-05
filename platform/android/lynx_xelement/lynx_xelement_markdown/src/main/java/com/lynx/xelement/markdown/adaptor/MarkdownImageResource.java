// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.graphics.Bitmap;
import android.graphics.drawable.Animatable;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.image.ImageUrlRedirectUtils;
import com.lynx.tasm.image.ImageContent;
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
  private @Nullable ImageContent mImageContent;
  private @Nullable ImageRequestInfo mRequestInfo;

  public MarkdownImageResource(
      String source, boolean downSampling, boolean syncLoading, MarkdownResourceContext host) {
    mSource = source;
    mHost = host;
    UIThreadUtils.runOnUiThreadImmediately(() -> fetchImage(downSampling));
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
        if (mDrawable instanceof Animatable) {
          ((Animatable) mDrawable).stop();
        }
        Drawable.Callback callback = mHost.getDrawableCallback();
        if (callback instanceof LynxServalViewWrapper) {
          ((LynxServalViewWrapper) callback).unregisterAnimatedDrawable(mDrawable);
        }
        mDrawable.setCallback(null);
        mDrawable = null;
      }
      mImageContent = null;
      if (mRequestInfo != null) {
        ILynxImageService service = LynxServiceCenter.inst().getService(ILynxImageService.class);
        if (service != null) {
          service.releaseImage(mRequestInfo);
        }
        mRequestInfo = null;
      }
    });
  }

  private void fetchImage(boolean downSampling) {
    try {
      LynxContext lynxContext = mHost.getLynxContext();
      if (lynxContext == null) {
        return;
      }
      ILynxImageService service = LynxServiceCenter.inst().getService(ILynxImageService.class);
      if (service == null) {
        mHost.onImageLoadError(mSource, null);
        return;
      }

      String source = ImageUrlRedirectUtils.redirectUrl(lynxContext, mSource);
      if (TextUtils.isEmpty(source)) {
        mHost.onImageLoadError(mSource, null);
        return;
      }

      ImageRequestInfoBuilder builder = ImageRequestInfoBuilder.newBuilderWithSource(source);
      if (downSampling) {
        builder.setResizeWidth(IMAGE_DOWNSAMPLE_SIZE)
            .setResizeHeight(IMAGE_DOWNSAMPLE_SIZE)
            .setEnableDownSampling(true);
      } else {
        builder.setEnableDownSampling(false);
      }
      builder.setCallerContext(lynxContext.getFrescoCallerContext());
      mRequestInfo = builder.build();

      service.fetchImage(mRequestInfo, new ImageLoadListener() {
        @Override
        public void onRequestSubmit(ImageRequestInfo imageRequestInfo) {}

        @Override
        public void onSuccess(@Nullable ImageContent imageContent, ImageRequestInfo requestInfo,
            ImageInfo imageInfo) {
          if (imageContent != null && imageInfo != null && imageInfo.getWidth() > 0
              && imageInfo.getHeight() > 0) {
            mImageContent = imageContent;
            Drawable drawable = imageContent.getDrawable();
            if (drawable == null) {
              Bitmap bitmap = imageContent.getBitmap();
              if (bitmap != null && !bitmap.isRecycled()) {
                drawable = new BitmapDrawable(null, bitmap);
              }
            }
            if (drawable != null) {
              drawable.setBounds(0, 0, imageInfo.getWidth(), imageInfo.getHeight());
              mDrawable = drawable;
              Drawable.Callback viewCallback = mHost.getDrawableCallback();
              if (viewCallback != null) {
                drawable.setCallback(viewCallback);
                if (drawable instanceof Animatable
                    && viewCallback instanceof LynxServalViewWrapper) {
                  ((LynxServalViewWrapper) viewCallback)
                      .registerAnimatedDrawable(drawable, mSource);
                  ((Animatable) drawable).start();
                }
              }
            }
          }
          mHost.onImageLoaded(mSource);
        }

        @Override
        public void onFailure(int errorCode, Throwable throwable) {
          mHost.onImageLoadError(mSource, throwable);
        }

        @Override
        public void onImageMonitorInfo(JSONObject monitorInfo) {}
      }, null, lynxContext);
    } catch (Exception e) {
      mHost.onImageLoadError(mSource, e);
    } finally {
      mLoadLatch.countDown();
    }
  }
}
