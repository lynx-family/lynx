// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.shell;

import com.lynx.tasm.LynxLoadMeta;
import com.lynx.tasm.LynxView;
import java.util.Map;

public class HttpTemplateDispatcher extends TemplateDispatcher {
  private static final String HTTP_URL_PREFIX = "http://";
  private static final String HTTPS_URL_PREFIX = "https://";

  @Override
  public boolean checkUrl(String url) {
    return url.startsWith(HTTP_URL_PREFIX) || url.startsWith(HTTPS_URL_PREFIX);
  }
}
