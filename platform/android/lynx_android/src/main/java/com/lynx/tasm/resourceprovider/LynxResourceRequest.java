// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider;

import com.lynx.tasm.behavior.LynxContext;
import java.util.Map;

/**
 * Request Object that identifies the Current Resource Request.
 */
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
    LynxResourceTypeExternalJSSource,
    LynxResourceTypeExternalByteCode,
  }

  public enum AsyncMode {
    EXACTLY_ASYNC, // async resource request required.
    EXACTLY_SYNC, // sync resource request required.
    MOST_SYNC // sync resource request if offline file existed, otherwise async.
  }

  private final String url;
  private final LynxResourceType resourceType;
  private final LynxContext lynxContext;
  private Map<String, Object> params;
  private AsyncMode asyncMode;

  /**
   * Construct a resource request object.
   * @param url url of the requesting resource
   * @param type type of the requesting resource
   */
  public LynxResourceRequest(String url, LynxResourceType type) {
    this(url, type, null);
  }

  /**
   * Construct a resource request object.
   * @param url url of the requesting resource
   * @param type type of the requesting resource
   * @param context the current lynxContext instance;
   */
  public LynxResourceRequest(String url, LynxResourceType type, LynxContext context) {
    this.url = url;
    this.resourceType = type;
    this.lynxContext = context;
  }

  /**
   * Set the mode of current resource request
   * @param mode
   *  - EXACTLY_ASYNC: This request should be requested asynchronously
   *  - EXACTLY_SYNC: This request should be requested synchronously
   *  - MOST_SYNC: sync resource request if offline file existed, otherwise async.
   */
  public void setAsyncMode(AsyncMode mode) {
    this.asyncMode = mode;
  }

  /**
   * Set the additional params of the current resource request
   * @param params the additional params
   */
  public void setParams(Map<String, Object> params) {
    this.params = params;
  }

  /**
   * Get the url of the requesting resource
   * @return resource url
   */
  public String getUrl() {
    return this.url;
  }

  /**
   * Get the type of the requesting resource
   * @return resource type
   */
  public LynxResourceType getResourceType() {
    return this.resourceType;
  }

  /**
   * Get the thread mode of the requesting resource
   * @return thread mode
   */
  public AsyncMode getAsyncMode() {
    return this.asyncMode;
  }

  /**
   * Get the additional params of the current resource request.
   * @return the additional params
   */
  public Map<String, Object> getParams() {
    return this.params;
  }

  /**
   * get the current lynxContext instance;
   * @return LynxContext
   */
  public LynxContext getLynxContext() {
    return this.lynxContext;
  }
}
