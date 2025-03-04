// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import android.content.Context;
import android.graphics.drawable.Drawable;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.image.model.AnimationListener;
import com.lynx.tasm.image.model.ImageLoadListener;
import com.lynx.tasm.image.model.ImageRequestInfo;
import com.lynx.tasm.image.model.LynxImageFetcher;
import com.lynx.tasm.service.ILynxImageService;
import com.lynx.tasm.service.LynxServiceCenter;

// Image loading delegate class  for encapsulating the implementation of image resource loading and
// animated image playback control.
class LynxImageLoader {
  private ILynxImageService mLynxImageService;
  private LynxImageFetcher mImageFetcher;
  private boolean mEnableImageFetcher;

  public LynxImageLoader(LynxImageFetcher imageFetcher) {
    mImageFetcher = imageFetcher;
    mLynxImageService = LynxServiceCenter.inst().getService(ILynxImageService.class);
    mEnableImageFetcher = imageFetcher != null;
  }

  public void fetchImage(@NonNull ImageRequestInfo imageRequestInfo,
      @NonNull ImageLoadListener loadListener, @Nullable AnimationListener animationListener,
      Context context) {
    String section = "LynxImageServiceProxy.fetchImage";
    TraceEvent.beginSection(section);
    if (mEnableImageFetcher) {
      mImageFetcher.loadImage(imageRequestInfo, loadListener, animationListener, context);
    } else {
      mLynxImageService.fetchImage(imageRequestInfo, loadListener, animationListener, context);
    }
    TraceEvent.endSection(section);
  }

  public boolean startAnimation(Drawable animatable) {
    if (!mEnableImageFetcher) {
      return mLynxImageService.startAnimation(animatable);
    }
    return false;
  }

  public boolean resumeAnimation(Drawable animatable) {
    if (!mEnableImageFetcher) {
      return mLynxImageService.resumeAnimation(animatable);
    }
    return false;
  }

  public boolean pauseAnimation(Drawable animatable) {
    if (!mEnableImageFetcher) {
      return mLynxImageService.pauseAnimation(animatable);
    }
    return false;
  }

  public boolean stopAnimation(Drawable animatable) {
    if (!mEnableImageFetcher) {
      return mLynxImageService.stopAnimation(animatable);
    }
    return false;
  }

  public void releaseImage(ImageRequestInfo imageRequestInfo) {
    if (!mEnableImageFetcher) {
      mLynxImageService.releaseImage(imageRequestInfo);
    } else {
      mImageFetcher.releaseImage(imageRequestInfo);
    }
  }

  public void releaseAnimDrawable(Drawable drawable) {
    if (!mEnableImageFetcher) {
      mLynxImageService.releaseAnimDrawable(drawable);
    }
  }
}
