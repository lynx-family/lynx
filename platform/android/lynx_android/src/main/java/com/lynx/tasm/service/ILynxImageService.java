// Copyright 2024 The Lynx Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.lynx.tasm.service;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.ui.background.BackgroundLayerDrawable;
import com.lynx.tasm.image.model.AnimationListener;
import com.lynx.tasm.image.model.ImageLoadListener;
import com.lynx.tasm.image.model.ImageRequestInfo;

/**
 * Interface defining the Lynx image service with methods for image loading,
 * animation control, and resource management.
 */
public interface ILynxImageService extends IServiceProvider {
  /**
   * Get service class, DO NOT OVERRIDE THIS METHOD
   */
  @NonNull
  default Class<? extends IServiceProvider> getServiceClass() {
    return ILynxImageService.class;
  }
  /**
   * Image loading implementation that returns either a BitmapDrawable or an AnimDrawable based on
   * the imageRequest.
   *
   * @param imageRequestInfo Information about the image request.
   * @param loadListener Listener for image loading callbacks.
   * @param animationListener Listener for animation callbacks (optional).
   * @param context The Lynx context.
   */
  void fetchImage(@NonNull ImageRequestInfo imageRequestInfo,
      @NonNull ImageLoadListener loadListener, @Nullable AnimationListener animationListener,
      @NonNull Context context);

  /**
   * Controls the playback initiation of the AnimDrawable.
   *
   * @param animatable The drawable to be animated.
   * @return True if the animation starts successfully, false otherwise.
   */
  boolean startAnimation(@NonNull Drawable animatable);

  /**
   * Resumes the animation of an AnimDrawable.
   *
   * @param animatable The drawable to be animated.
   * @return True if the animation resumes successfully, false otherwise.
   */
  boolean resumeAnimation(@NonNull Drawable animatable);

  /**
   * Pauses the animation of an AnimDrawable.
   *
   * @param animatable The drawable to be paused.
   * @return True if the animation pauses successfully, false otherwise.
   */
  boolean pauseAnimation(@NonNull Drawable animatable);

  /**
   * Stops the animation of an AnimDrawable.
   *
   * @param animatable The drawable to be stopped.
   * @return True if the animation stops successfully, false otherwise.
   */
  boolean stopAnimation(@NonNull Drawable animatable);

  /**
   * Prefetches an image from a URI for faster subsequent loading.
   *
   * @param uri The URI of the image to prefetch.
   * @param callerContext Optional context of the caller.
   * @param params Additional parameters for prefetching (optional).
   */
  void prefetchImage(
      @NonNull String uri, @Nullable Object callerContext, @Nullable ReadableMap params);

  /**
   * Releases resources associated with the given image request.
   *
   * @param imageRequestInfo Information about the image request to release.
   */
  void releaseImage(@NonNull ImageRequestInfo imageRequestInfo);

  /**
   * Releases resources associated with an AnimDrawable.
   *
   * @param drawable The drawable to be released.
   */
  void releaseAnimDrawable(@NonNull Drawable drawable);

  /**
   * The implementation of background-image: url
   * now provides a default {@link com.lynx.tasm.behavior.ui.image.BackgroundImageDrawable}
   * implementation.
   *
   * @param context The Android context.
   * @param url The URL of the background image.
   * @return A BackgroundLayerDrawable representing the background image.
   */
  @Nullable
  BackgroundLayerDrawable createBackgroundImageDrawable(
      @NonNull Context context, @NonNull String url);

  /**
   * Deprecated and does not require implementation.
   *
   * @param builder The custom image decoder builder.
   */
  @Deprecated void setCustomImageDecoder(@NonNull Object builder);

  /**
   * Deprecated and does not require implementation.
   *
   * @return The post-processor for super-resolution images (optional).
   */
  @Deprecated @Nullable Object getImageSRPostProcessor();

  /**
   * Deprecated and does not require implementation.
   *
   * @param request The image request object.
   * @param view The view to be used for size adjustment.
   */
  @Deprecated void setImageSRSize(@NonNull Object request, @NonNull View view);

  /**
   * Deprecated and does not require implementation.
   *
   * @param cacheChoice The cache choice for the image.
   * @param builder The builder object.
   */
  @Deprecated void setImageCacheChoice(@NonNull String cacheChoice, @NonNull Object builder);

  /**
   * Deprecated and does not require implementation.
   *
   * @param hierarchy The hierarchy object.
   * @param request The image request object.
   * @param scaleType The scale type for the image.
   * @param hash The placeholder hash.
   * @param metaData Optional metadata.
   * @param width The placeholder width.
   * @param height The placeholder height.
   * @param radius The radius for the placeholder.
   * @param iterations The number of iterations for processing.
   * @param isPreView Indicates if it is a preview.
   */
  @Deprecated
  void setImagePlaceHolderHash(@NonNull Object hierarchy, @NonNull Object request,
      @NonNull Object scaleType, @NonNull String hash, @Nullable String metaData, int width,
      int height, int radius, int iterations, boolean isPreView);
}
