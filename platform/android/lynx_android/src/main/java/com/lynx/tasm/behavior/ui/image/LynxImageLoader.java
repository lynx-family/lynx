// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import android.content.Context;
import android.graphics.drawable.Drawable;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.base.trace.TraceEventDef;
import com.lynx.tasm.image.model.AnimationListener;
import com.lynx.tasm.image.model.ImageLoadListener;
import com.lynx.tasm.image.model.ImageRequestInfo;
import com.lynx.tasm.image.model.LynxImageFetcher;
import com.lynx.tasm.service.ILynxImageService;
import com.lynx.tasm.service.LynxServiceCenter;

// Image loading delegate class  for encapsulating the implementation of image resource loading and
// animated image playback control.
class LynxImageLoader {
  @Nullable private volatile ILynxImageService mLynxImageService;
  private LynxImageFetcher mImageFetcher;
  private boolean mEnableImageFetcher;

  public LynxImageLoader(LynxImageFetcher imageFetcher) {
    mImageFetcher = imageFetcher;
    mLynxImageService = LynxServiceCenter.inst().getService(ILynxImageService.class);
    mEnableImageFetcher = imageFetcher != null;
  }

  @Nullable
  private ILynxImageService getImageService() {
    if (mLynxImageService == null) {
      mLynxImageService = LynxServiceCenter.inst().getService(ILynxImageService.class);
    }
    return mLynxImageService;
  }

  public void fetchImage(@NonNull ImageRequestInfo imageRequestInfo,
      @NonNull ImageLoadListener loadListener, @Nullable AnimationListener animationListener,
      Context context) {
    TraceEvent.beginSection(TraceEventDef.IMAGE_SERVICE_PROXY_FETCH_IMAGE);
    if (mEnableImageFetcher) {
      mImageFetcher.loadImage(imageRequestInfo, loadListener, animationListener, context);
    } else {
      ILynxImageService imageService = getImageService();
      if (imageService == null) {
        loadListener.onFailure(LynxSubErrorCode.E_RESOURCE_IMAGE_EXCEPTION,
            new IllegalStateException("Lynx image service is unavailable."));
      } else {
        imageService.fetchImage(imageRequestInfo, loadListener, animationListener, context);
      }
    }
    TraceEvent.endSection(TraceEventDef.IMAGE_SERVICE_PROXY_FETCH_IMAGE);
  }

  public boolean startAnimation(Drawable animatable) {
    if (!mEnableImageFetcher) {
      ILynxImageService imageService = getImageService();
      return imageService != null && imageService.startAnimation(animatable);
    }
    return false;
  }

  public boolean resumeAnimation(Drawable animatable) {
    if (!mEnableImageFetcher) {
      ILynxImageService imageService = getImageService();
      return imageService != null && imageService.resumeAnimation(animatable);
    }
    return false;
  }

  public boolean pauseAnimation(Drawable animatable) {
    if (!mEnableImageFetcher) {
      ILynxImageService imageService = getImageService();
      return imageService != null && imageService.pauseAnimation(animatable);
    }
    return false;
  }

  public boolean stopAnimation(Drawable animatable) {
    if (!mEnableImageFetcher) {
      ILynxImageService imageService = getImageService();
      return imageService != null && imageService.stopAnimation(animatable);
    }
    return false;
  }

  public void releaseImage(ImageRequestInfo imageRequestInfo) {
    if (!mEnableImageFetcher) {
      ILynxImageService imageService = getImageService();
      if (imageService != null) {
        imageService.releaseImage(imageRequestInfo);
      }
    } else {
      mImageFetcher.releaseImage(imageRequestInfo);
    }
  }

  public void releaseAnimDrawable(Drawable drawable) {
    if (!mEnableImageFetcher) {
      ILynxImageService imageService = getImageService();
      if (imageService != null) {
        imageService.releaseAnimDrawable(drawable);
      }
    }
  }

  public boolean canParseUrl(String url) {
    if (!mEnableImageFetcher) {
      ILynxImageService imageService = getImageService();
      return imageService != null && imageService.canParseUrl(url);
    }
    return false;
  }
}
