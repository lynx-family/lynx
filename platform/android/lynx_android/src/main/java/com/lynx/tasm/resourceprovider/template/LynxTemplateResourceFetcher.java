// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider.template;

import com.lynx.tasm.resourceprovider.LynxResourceCallback;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.LynxResourceResponse;

public abstract class LynxTemplateResourceFetcher {
  /**
   * fetch template resource of lynx & dynamic component etc.
   *
   * @param request
   * @param callback response with the requiring content file: byteArray or TemplateBundle
   */
  public abstract void fetchTemplate(
      LynxResourceRequest request, LynxResourceCallback<TemplateProviderResult> callback);

  /**
   * fetch SSRData of lynx.
   *
   * @param request
   * @param callback response with the requiring ssr data.
   */
  public abstract void fetchSSRData(
      LynxResourceRequest request, LynxResourceCallback<byte[]> callback);
}
