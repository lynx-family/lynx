// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider;

import java.util.Map;

public final class LynxResourceRequest {
  public enum LynxResourceType {
    LynxResourceTypeGeneric,
    LynxResourceTypeImage,
    LynxResourceTypeFont,
    LynxResourceTypeLottie,
    LynxResourceTypeVideo,
    LynxResourceTypeSVG,
    LynxResourceTypeTemplate,
    LynxResourceTypeLynxCoreJS,
    LynxResourceTypeDynamicComponent,
    LynxResourceTypeI18NText,
    LynxResourceTypeTheme,
    LynxResourceTypeExternalJSSource
  }

  public enum AsyncMode {
    EXACTLY_ASYNC, // async resource request required.
    EXACTLY_SYNC, // sync resource request required.
    MOST_SYNC // sync resource request if offline file existed, otherwise async.
  }

  private final String url;
  private final LynxResourceType resourceType;
  private Map<String, Object> params;
  private AsyncMode asyncMode;

  public LynxResourceRequest(String url, LynxResourceType type) {
    this.url = url;
    this.resourceType = type;
  }

  public String getUrl() {
    return this.url;
  }

  public LynxResourceType getResourceType() {
    return this.resourceType;
  }

  public void setAsyncMode(AsyncMode mode) {
    this.asyncMode = mode;
  }

  public AsyncMode getAsyncMode() {
    return this.asyncMode;
  }

  public void setParams(Map<String, Object> params) {
    this.params = params;
  }

  public Map<String, Object> getParams() {
    return this.params;
  }
}
