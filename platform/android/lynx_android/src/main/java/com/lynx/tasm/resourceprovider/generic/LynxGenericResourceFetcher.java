// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider.generic;

import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;

/**
 * @apidoc
 * @brief `LynxGenericResourceFetcher` is defined inside `LynxEngine`
 * and injected from outside to implement a general resource loading interface.
 * It is used inside LynxEngine for resource loading capabilities of components such as `Text`
 */
public abstract class LynxGenericResourceFetcher {
  /**
   * @apidoc
   * @brief `LynxEngine` internally calls this method to obtain the general
   * resource content, and the return result is required to be the resource content `byte[]` type.
   *
   * @param request Request for the requiring resource.
   * @param callback Contents of the requiring resource.
   * @note This method must be implemented.
   */
  public abstract void fetchResource(
      LynxResourceRequest request, LynxResourceCallback<byte[]> callback);

  /**
   * @apidoc
   * @brief `LynxEngine` internally calls this method to obtain the path of the common
   * resource on the local disk, and the return result is required to be of `String` type.
   *
   * @param request Request for the requiring resource.
   * @param callback Path on the disk of the requiring resource.
   * @note This method must be implemented.
   */
  public abstract void fetchResourcePath(
      LynxResourceRequest request, LynxResourceCallback<String> callback);

  /**
   * @apidoc
   * @brief `LynxEngine` internally calls this method to obtain the bytecode content,
   * and the return result is required to be the resource content `byte[]` type.
   *
   * @param request Request for the requiring resource.
   * @param callback Contents of the requiring resource.
   * @note This method is optional to be implemented.
   *
   * @Since 3.6
   */
  public void fetchBytecode(LynxResourceRequest request, LynxResourceCallback<byte[]> callback) {
    callback.onResponse(
        LynxResourceResponse.onFailed(new Throwable("fetchBytecode is not implemented.")));
  }

  /**
   * @apidoc
   * @brief `LynxEngine` internally calls this method to obtain resource content in a streaming
   * manner.
   *
   * @param request Request for the requiring resource.
   * @param delegate Streaming of the requiring resource.
   * @note This method is optional to be implemented.
   */
  public void fetchStream(LynxResourceRequest request, StreamDelegate delegate){};

  /**
   * @apidoc
   * @brief Cancel the request of the requiring resource.
   *
   * @param request the requiring request.
   * @note This method is optional to be implemented.
   */
  public void cancel(LynxResourceRequest request){};
}
