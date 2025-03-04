// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider.generic;

import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;

/**
 * Generic Resource Provider for Lynx.
 */
public abstract class LynxGenericResourceFetcher {
  /**
   * fetch resource with contents.
   *
   * @param request
   * @param callback contents of the requiring resource.
   */
  public abstract void fetchResource(
      LynxResourceRequest request, LynxResourceCallback<byte[]> callback);

  /**
   * fetch resource with res path.
   *
   * @param request
   * @param callback path on the disk of the requiring resource.
   */
  public abstract void fetchResourcePath(
      LynxResourceRequest request, LynxResourceCallback<String> callback);

  /**
   * fetch resource with stream.
   *
   * @param request
   * @param delegate streaming of the requiring resource.
   */
  public void fetchStream(LynxResourceRequest request, StreamDelegate delegate){};

  /**
   * cancel the request of the requiring resource.
   *
   * @param request the requiring request.
   */
  public void cancel(LynxResourceRequest request){};
}
