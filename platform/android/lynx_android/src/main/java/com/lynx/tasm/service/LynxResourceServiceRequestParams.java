// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service;

import java.util.ArrayList;
import java.util.Arrays;

public class LynxResourceServiceRequestParams {
  // resource usage scenarios
  private LynxServiceScene resourceScene = LynxServiceScene.OTHER;

  // support memory cache
  private Boolean enableMemoryCache = null;

  private Boolean enableRequestReuse = false;

  /**
   * The url of the request.
   */
  private String templateUrl = null;

  public LynxServiceScene getResourceScene() {
    return resourceScene;
  }

  public void setResourceScene(LynxServiceScene resourceScene) {
    this.resourceScene = resourceScene;
  }

  public Boolean getEnableMemoryCache() {
    return enableMemoryCache;
  }

  public void setEnableMemoryCache(Boolean enableMemoryCache) {
    this.enableMemoryCache = enableMemoryCache;
  }

  public Boolean getEnableRequestReuse() {
    return enableRequestReuse;
  }

  public void setEnableRequestReuse(Boolean enableRequestReuse) {
    this.enableRequestReuse = enableRequestReuse;
  }

  public String getTemplateUrl() {
    return templateUrl;
  }

  public void setTemplateUrl(String templateUrl) {
    this.templateUrl = templateUrl;
  }

  /**
   * resource usage scenarios
   */
  public enum LynxServiceScene {
    // lynx's template
    LYNX_TEMPLATE,
    // lynx's external js
    LYNX_EXTERNAL_JS,
    // lynx's DynamicComponentFetcher
    LYNX_COMPONENT,
    // lynx's font provider
    LYNX_FONT,
    // lynx's i18n text
    LYNX_I18N,
    // lynx's image
    LYNX_IMAGE,
    // lynx's lottie resource
    LYNX_LOTTIE,
    LYNX_VIDEO,
    LYNX_SVG,
    // lynx's child resource, but can't determine specific type
    LYNX_CHILD_RESOURCE,
    // web's main resource, the first html
    WEB_MAIN_RESOURCE,
    // web's other resource, like css,js
    WEB_CHILD_RESOURCE,
    PRELOAD_CONFIG,

    // other resource
    OTHER
  }
}
