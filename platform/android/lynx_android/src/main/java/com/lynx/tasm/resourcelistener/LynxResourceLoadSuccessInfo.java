// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourcelistener;

public class LynxResourceLoadSuccessInfo {
  private String mUrl;

  public LynxResourceLoadSuccessInfo(String url) {
    mUrl = url;
  }

  public String getUrl() {
    return mUrl;
  }
}
