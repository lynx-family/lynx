// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.provider;

import android.text.TextUtils;
import java.util.HashMap;
import java.util.Map;

/**
 * Lynx Resource Provider Manager
 */
public class LynxProviderRegistry {
  public static final String LYNX_PROVIDER_TYPE_IMAGE = "IMAGE";
  public static final String LYNX_PROVIDER_TYPE_FONT = "FONT";
  public static final String LYNX_PROVIDER_TYPE_LOTTIE = "LOTTIE";
  public static final String LYNX_PROVIDER_TYPE_VIDEO = "VIDEO";
  public static final String LYNX_PROVIDER_TYPE_SVG = "SVG";
  public static final String LYNX_PROVIDER_TYPE_TEMPLATE = "TEMPLATE";
  public static final String LYNX_PROVIDER_TYPE_LYNX_CORE_JS = "LYNX_CORE_JS";
  public static final String LYNX_PROVIDER_TYPE_DYNAMIC_COMPONENT = "DYNAMIC_COMPONENT";
  public static final String LYNX_PROVIDER_TYPE_I18N_TEXT = "I18N_TEXT";
  public static final String LYNX_PROVIDER_TYPE_THEME = "THEME";
  // for external js source provider
  public static final String LYNX_PROVIDER_TYPE_EXTERNAL_JS = "EXTERNAL_JS_SOURCE";
  final Map<String, LynxResourceProvider> resourceProviders = new HashMap<>();

  public void addLynxResourceProvider(String key, LynxResourceProvider provider) {
    if (!TextUtils.isEmpty(key)) {
      resourceProviders.put(key, provider);
    }
  }

  public LynxResourceProvider getProviderByKey(String key) {
    return TextUtils.isEmpty(key) ? null : resourceProviders.get(key);
  }

  public void clear() {
    resourceProviders.clear();
  }
}
