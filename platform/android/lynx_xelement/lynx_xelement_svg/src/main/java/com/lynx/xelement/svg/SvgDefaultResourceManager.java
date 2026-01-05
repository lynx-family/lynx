// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.svg;

import android.graphics.Bitmap;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import com.facebook.common.executors.UiThreadImmediateExecutorService;
import com.facebook.common.references.CloseableReference;
import com.facebook.datasource.DataSource;
import com.facebook.drawee.backends.pipeline.Fresco;
import com.facebook.imagepipeline.common.ImageDecodeOptionsBuilder;
import com.facebook.imagepipeline.core.ImagePipeline;
import com.facebook.imagepipeline.datasource.BaseBitmapDataSubscriber;
import com.facebook.imagepipeline.image.CloseableImage;
import com.facebook.imagepipeline.request.ImageRequest;
import com.facebook.imagepipeline.request.ImageRequestBuilder;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.image.ImageUrlRedirectUtils;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

public class SvgDefaultResourceManager {
  private static final String TAG = "SvgResourceManager";

  private WeakReference<LynxUISVG> mWeakUI = null;

  public interface BitmapLoadCallback {
    void onSuccess(Bitmap bitmap);

    void onFailed();
  }

  private LynxContext mLynxContext;
  private HashMap<String, Bitmap> mBitmaps;
  public SvgDefaultResourceManager(LynxContext lynxContext) {
    this.mLynxContext = lynxContext;
    mBitmaps = new HashMap<>();
  }

  public SvgDefaultResourceManager(LynxContext lynxContext, LynxUISVG ui) {
    this.mLynxContext = lynxContext;
    mBitmaps = new HashMap<>();
    mWeakUI = new WeakReference<>(ui);
  }

  public void destroy() {
    for (Map.Entry<String, Bitmap> bm : mBitmaps.entrySet()) {
      try {
        bm.getValue().recycle();
      } catch (Throwable e) {
        e.printStackTrace();
        Log.e(TAG, e.toString());
      }
    }
    mBitmaps.clear();
  }

  @WorkerThread
  @Nullable
  public void requestBitmapSync(@NonNull String url, final BitmapLoadCallback callback) {
    if (mBitmaps.containsKey(url)) {
      callback.onSuccess(mBitmaps.get(url));
      return;
    }
    final String rawUrl = url;
    url = ImageUrlRedirectUtils.redirectUrl(mLynxContext, url);
    if (TextUtils.isEmpty(url)) {
      Log.d(TAG, "url is empty!");
      callback.onFailed();
      return;
    }
    Uri uri = Uri.parse(url);

    ImageRequestBuilder imageRequestBuilder = ImageRequestBuilder.newBuilderWithSource(uri);

    ImageDecodeOptionsBuilder decodeOptionsBuilder =
        new ImageDecodeOptionsBuilder().setBitmapConfig(Bitmap.Config.ARGB_8888);
    imageRequestBuilder.setImageDecodeOptions(decodeOptionsBuilder.build());

    ImageRequest imageRequest = imageRequestBuilder.build();

    ImagePipeline imagePipeline = Fresco.getImagePipeline();
    final DataSource<CloseableReference<CloseableImage>> dataSource =
        imagePipeline.fetchDecodedImage(imageRequest, TAG);

    // make sure the bitmap is processed in UI thread
    dataSource.subscribe(new BaseBitmapDataSubscriber() {
      @Override
      public void onNewResultImpl(@Nullable Bitmap bitmap) {
        if (dataSource.isFinished() && bitmap != null) {
          Bitmap bm = Bitmap.createBitmap(bitmap);
          mBitmaps.put(rawUrl, bm);
          dataSource.close();
          if (mWeakUI != null) {
            LynxUISVG ui = mWeakUI.get();
            if (ui != null) {
              ui.invalidateSVG();
            }
          }
        }
      }

      @Override
      public void onFailureImpl(DataSource dataSource) {
        callback.onFailed();
        if (dataSource != null) {
          dataSource.close();
        }
      }
    }, UiThreadImmediateExecutorService.getInstance());
  }
}
