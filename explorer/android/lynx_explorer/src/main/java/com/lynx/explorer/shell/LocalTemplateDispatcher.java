// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.shell;

public class LocalTemplateDispatcher extends TemplateDispatcher {
  private static final String LOCAL_PATH_PREFIX = "assets://";
  private static final String LOCAL_URL_PREFIX = "file://lynx?local://";

  @Override
  boolean checkUrl(String url) {
    return url.startsWith(LOCAL_PATH_PREFIX) || url.startsWith(LOCAL_URL_PREFIX);
  }
}
