// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.provider;

import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import com.lynx.tasm.resourceprovider.media.LynxMediaResourceFetcher;

public class DemoMediaResourceFetcher extends LynxMediaResourceFetcher {
  @Override
  public String shouldRedirectUrl(LynxResourceRequest request) {
    // in example just return input path.
    return request.getUrl();
  }
}
