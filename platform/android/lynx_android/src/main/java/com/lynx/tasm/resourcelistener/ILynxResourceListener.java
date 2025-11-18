// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourcelistener;

import com.lynx.tasm.resourceprovider.LynxResourceRequest;

public interface ILynxResourceListener {
  /**
   * Called immediately before a resource request is submitted
   * @param url the resource URL being requested
   * @param type the category of resource being requested
   */
  void onResourceLoadStart(String url, LynxResourceRequest.LynxResourceType type);

  /**
   * Called when a resource request completes successfully.
   * @param url the requested resource URL
   * @param type the category of the loaded resource
   * @param reportInfo contains detailed information about the successful load, including timing
   *     metrics and resource metadata
   */
  void onResourceLoadSuccess(String url, LynxResourceRequest.LynxResourceType type,
      LynxResourceLoadSuccessInfo reportInfo);

  /**
   * Called when a resource request fails to complete.
   * @param url the requested resource URL
   * @param type the category of the failed resource
   * @param errorInfo contains failure details including error code, message and context for
   *     debugging
   */
  void onResourceLoadFailed(
      String url, LynxResourceRequest.LynxResourceType type, LynxResourceLoadFailedInfo errorInfo);
}
