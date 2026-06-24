// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax.loader;

import androidx.annotation.NonNull;
import com.lynx.animax.ability.BaseAbility;
import com.lynx.animax.service.IAnimaXImageService;
import com.lynx.animax.util.AnimaXLog;
import com.lynx.animax.util.DeviceUtil;
import com.lynx.animax.util.LynxResourceUtil;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.core.ResManager;
import com.lynx.tasm.provider.LynxResCallback;
import com.lynx.tasm.provider.LynxResRequest;
import com.lynx.tasm.provider.LynxResResponse;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import java.lang.ref.WeakReference;

public class LynxHttpAnimaXLoader implements IAnimaXLoader {
  private static final String TAG = "LynxHttpAnimaXLoader";

  private final WeakReference<BaseAbility> mAbility;
  private final WeakReference<LynxContext> mLynxContext;
  private final boolean mUseLegacyFetcher;

  public LynxHttpAnimaXLoader(@NonNull BaseAbility ability, @NonNull LynxContext context) {
    mAbility = new WeakReference<>(ability);
    mLynxContext = new WeakReference<>(context);

    mUseLegacyFetcher = DeviceUtil.useLegacyFetcher(ability);
    if (mUseLegacyFetcher || context.getGenericResourceFetcher() == null) {
      AnimaXLog.i(TAG,
          "Generic fetcher unavailable, mUseLegacyFetcher: " + mUseLegacyFetcher
              + ", genericFetcher is null:" + (context.getGenericResourceFetcher() == null));
    }
  }

  @Override
  public void load(IAnimaXLoaderRequest request, IAnimaXLoaderCompletionHandler completionHandler) {
    // Handle image loading with image service if request contains image info
    if (handleImageRequest(request, completionHandler)) {
      return;
    }

    // Try to load resource with generic fetcher first (since Lynx 3.2)
    if (tryLoadWithGenericFetcher(request, completionHandler)) {
      return;
    }

    // Fallback to legacy loader if generic fetcher fails or not available
    loadWithLegacyLoader(request, completionHandler);
  }

  /**
   * Handle image loading request with image service if applicable
   * @return true if request was handled as image request
   */
  private boolean handleImageRequest(
      IAnimaXLoaderRequest request, IAnimaXLoaderCompletionHandler completionHandler) {
    if (request.getImageInfo() == null) {
      return false;
    }

    String url = request.getUri();
    BaseAbility ability = mAbility.get();
    if (ability != null) {
      url = ability.redirectUrl(url);
    }

    AnimaXLoaderRequest redirectedRequest = new AnimaXLoaderRequest(url, request.getParams());
    IAnimaXImageService imageService =
        ability != null ? ability.getService(IAnimaXImageService.class) : null;
    if (imageService != null && imageService.loadImage(redirectedRequest, completionHandler)) {
      return true;
    }

    completionHandler.onComplete(AnimaXLoaderResponse.createErrorResponse(
        new Throwable("Image loader service unavailable")));
    return true;
  }

  /**
   * Try to load resource using generic fetcher
   * @return true if request was successfully initiated
   */
  private boolean tryLoadWithGenericFetcher(
      IAnimaXLoaderRequest request, IAnimaXLoaderCompletionHandler completionHandler) {
    if (mUseLegacyFetcher) {
      return false;
    }

    // Return false when context or fetcher is null.
    // This allows fallback to the legacy loader when the generic loader is unavailable.
    LynxContext lynxContext = mLynxContext.get();
    if (lynxContext == null) {
      AnimaXLog.i(TAG, "Generic fetcher unavailable: context is null");
      return false;
    }

    LynxGenericResourceFetcher fetcher = lynxContext.getGenericResourceFetcher();
    if (fetcher == null) {
      return false;
    }

    LynxResourceRequest lynxRequest = new LynxResourceRequest(
        request.getUri(), LynxResourceRequest.LynxResourceType.LynxResourceTypeLottie);

    fetcher.fetchResource(lynxRequest, new LynxResourceCallback<byte[]>() {
      @Override
      public void onResponse(LynxResourceResponse<byte[]> response) {
        byte[] rawData = response.getData();
        if (rawData != null && response.getState() == LynxResourceResponse.ResponseState.SUCCESS) {
          completionHandler.onComplete(AnimaXLoaderResponse.createByteArrayResponse(rawData));
        } else {
          completionHandler.onComplete(
              AnimaXLoaderResponse.createErrorResponse(response.getError()));
        }
      }
    });
    return true;
  }

  /**
   * Load resource using legacy loader
   */
  private void loadWithLegacyLoader(
      IAnimaXLoaderRequest request, IAnimaXLoaderCompletionHandler completionHandler) {
    LynxContext lynxContext = mLynxContext.get();
    Object lynxExtraData = lynxContext != null ? lynxContext.getLynxExtraData() : null;
    LynxResRequest resRequest = new LynxResRequest(request.getUri(), lynxExtraData);
    LynxResCallback resCallback = new LynxResCallback() {
      @Override
      public void onSuccess(@NonNull LynxResResponse response) {
        byte[] result = LynxResourceUtil.getByteArrayFromLynxResResponse(response);
        AnimaXLoaderResponse<?> animaxResponse = null;
        if (result != null) {
          animaxResponse = AnimaXLoaderResponse.createByteArrayResponse(result);
        } else {
          animaxResponse = AnimaXLoaderResponse.createErrorResponse(
              new Throwable("Failed to load raw data with LynxResRequest"));
        }
        completionHandler.onComplete(animaxResponse);
      }

      @Override
      public void onFailed(LynxResResponse response) {
        AnimaXLoaderResponse<Throwable> animaxResponse = AnimaXLoaderResponse.createErrorResponse(
            new Throwable("LynxResRequest failed with error: " + response.getReasonPhrase()
                + "status code: " + response.getStatusCode()));
        completionHandler.onComplete(animaxResponse);
      }
    };

    ResManager.inst().requestResource(resRequest, resCallback);
  }

  @Override
  public AnimaXLoaderScheme getScheme() {
    return AnimaXLoaderScheme.HTTP;
  }
}
