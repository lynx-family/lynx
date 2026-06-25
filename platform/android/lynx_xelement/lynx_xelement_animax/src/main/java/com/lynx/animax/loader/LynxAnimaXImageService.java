// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax.loader;

import android.graphics.Bitmap;
import androidx.annotation.Nullable;
import com.lynx.animax.service.IAnimaXImageService;
import com.lynx.tasm.image.ImageContent;
import com.lynx.tasm.image.model.ImageInfo;
import com.lynx.tasm.image.model.ImageLoadListener;
import com.lynx.tasm.image.model.ImageRequestInfo;
import com.lynx.tasm.image.model.ImageRequestInfoBuilder;
import com.lynx.tasm.service.ILynxImageService;
import com.lynx.tasm.service.LynxServiceCenter;
import org.json.JSONObject;

public class LynxAnimaXImageService implements IAnimaXImageService {
  @Override
  public boolean loadImage(
      IAnimaXLoaderRequest request, IAnimaXLoaderCompletionHandler completionHandler) {
    ILynxImageService imageService = LynxServiceCenter.inst().getService(ILynxImageService.class);
    IAnimaXLoaderRequest.IImageInfo imageInfo = request.getImageInfo();
    if (imageService == null || imageInfo == null) {
      return false;
    }

    ImageRequestInfo imageRequestInfo =
        ImageRequestInfoBuilder.newBuilderWithSource(request.getUri())
            .setResizeWidth(imageInfo.getWidth())
            .setResizeHeight(imageInfo.getHeight())
            .setBitmapConfig(Bitmap.Config.ARGB_8888)
            .setForceStaticImage(true)
            .build();

    imageService.decodeImage(imageRequestInfo, new ImageLoadListener() {
      @Override
      public void onRequestSubmit(ImageRequestInfo imageRequestInfo) {}

      @Override
      public void onSuccess(
          @Nullable ImageContent imageContent, ImageRequestInfo requestInfo, ImageInfo imageInfo) {
        if (imageContent == null || imageContent.getBitmap() == null) {
          if (imageContent != null) {
            imageContent.releaseImageResource();
          }
          completionHandler.onComplete(
              AnimaXLoaderResponse.createErrorResponse(new Throwable("empty bitmap")));
          return;
        }

        completionHandler.onComplete(
            AnimaXLoaderResponse.createBitmapResponse(new LynxImageContentRef(imageContent)));
      }

      @Override
      public void onFailure(int errorCode, Throwable throwable) {
        completionHandler.onComplete(AnimaXLoaderResponse.createErrorResponse(
            throwable == null ? new Throwable("image load failed: " + errorCode) : throwable));
      }

      @Override
      public void onImageMonitorInfo(JSONObject monitorInfo) {}
    });

    return true;
  }

  private static final class LynxImageContentRef implements IAnimaXCloseableBitmapReference {
    private @Nullable ImageContent mImageContent;

    private LynxImageContentRef(ImageContent imageContent) {
      mImageContent = imageContent;
    }

    @Override
    public Bitmap get() {
      return mImageContent == null ? null : mImageContent.getBitmap();
    }

    @Override
    public void close() {
      if (mImageContent != null) {
        mImageContent.releaseImageResource();
        mImageContent = null;
      }
    }

    @Override
    public boolean isValid() {
      Bitmap bitmap = get();
      return bitmap != null && !bitmap.isRecycled();
    }
  }
}
