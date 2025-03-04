// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider.media;

import android.graphics.drawable.Drawable;
import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;
import java.io.Closeable;

public abstract class LynxMediaResourceFetcher {
  public abstract String shouldRedirectUrl(LynxResourceRequest request);

  /**
   * Quick check for a local path.
   *
   * @param url input path
   * @return
   *  TRUE if is a local path;
   *  FALSE if not a local path;
   *  UNDEFINED if not sure;
   */
  public OptionalBool isLocalResource(String url) {
    return OptionalBool.UNDEFINED;
  }

  /**
   * fetch Image Drawable directly.
   *
   * @param request
   * @param callback Response with the needed drawable.
   */
  public void fetchImage(LynxResourceRequest request, LynxResourceCallback<Closeable> callback) {}
}
