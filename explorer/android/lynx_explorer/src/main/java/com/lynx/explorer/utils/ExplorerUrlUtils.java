// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.utils;

/** URL compatibility helpers for routes that resolve to Explorer-packaged assets. */
public final class ExplorerUrlUtils {
  public static final String EXPLORER_LOCAL_PREFIX = "file://lynx?local://";
  public static final String LYNX_TEST_LOCAL_PREFIX = "sslocal://lynxtest?local://";

  private ExplorerUrlUtils() {}

  /**
   * Maps the canonical Lynx test route onto Explorer's existing local route. Everything after the
   * route prefix is copied verbatim so nested template queries keep their original encoding.
   */
  public static String normalizeLocalTestUrl(String url) {
    if (url != null && url.startsWith(LYNX_TEST_LOCAL_PREFIX)) {
      return EXPLORER_LOCAL_PREFIX + url.substring(LYNX_TEST_LOCAL_PREFIX.length());
    }
    return url;
  }
}
