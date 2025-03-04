// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import javax.xml.transform.Transformer;

public interface ImageInterceptor {
  /**
   * Notify the host application of a image request and allow the application
   * to redirect the url and return. If the return value is null, LynxView will
   * continue to load image from the origin url as usual. Otherwise, the redirect
   * url will be used.
   * <p>
   * This method will be called on any thread.
   * <p>
   * The following scheme is supported in LynxView:
   * 1. Http scheme: http:// or https://
   * 2. File scheme: file:// + path
   * 3. Assets scheme: asset:///
   * 4. Res scheme: res:///identifier or res:///image_name
   *
   * @param url the url that ready for loading image
   * @return A url string that fit int with the support scheme list or null
   */
  String shouldRedirectImageUrl(String url);

  interface CompletionHandler {
    /**
     * @param image must be CloseableReference<CloseableBitmap> or CloseableReference<Bitmap>
     */
    void imageLoadCompletion(@Nullable Object image, @Nullable Throwable throwable);
  }

  void loadImage(@NonNull Context context, @Nullable String cacheKey, @Nullable String src,
      float width, float height, final @Nullable Transformer transformer,
      @NonNull final CompletionHandler handler);
}
