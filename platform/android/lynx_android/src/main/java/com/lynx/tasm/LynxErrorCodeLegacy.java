// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

/**
 * ATTENTION !!!
 * This is a legacy error code file and will be deleted in the future.
 * Please use error behavior code in `LynxErrorBehavior.java` instead.
 */
@Deprecated
public class LynxErrorCodeLegacy {
  // Error code for no error.
  public static final int LYNX_ERROR_CODE_SUCCESS = LynxErrorBehavior.EB_SUCCESS;

  // Error occurred when loadTemplate, one should retry loadTemplate.
  public static final int LYNX_ERROR_CODE_LOAD_TEMPLATE = LynxErrorBehavior.EB_APP_BUNDLE_LOAD;
  // Error occurred when fetch template resource.
  public static final int LYNX_ERROR_CODE_TEMPLATE_PROVIDER = LynxErrorBehavior.EB_APP_BUNDLE_LOAD;

  // Error occurred when executing JavaScript code.
  public static final int LYNX_ERROR_CODE_JAVASCRIPT = LynxErrorBehavior.EB_BTS_RUNTIME_ERROR;
  // Error occurred when fetch resource. Check error message for more information.
  public static final int LYNX_ERROR_CODE_RESOURCE = LynxErrorBehavior.EB_RESOURCE_IMAGE;
  // Error occurred when Layout, one should retry or fallback
  public static final int LYNX_ERROR_CODE_LAYOUT = LynxErrorBehavior.EB_LAYOUT_INTERNAL;
  // Some unknown error in Update data pipeline. Check error message for more information.
  public static final int LYNX_ERROR_CODE_UPDATE = LynxErrorBehavior.EB_DATA_FLOW_UPDATE;
  // ReloadTemplate before loadTemplate, one should loadTemplate firstly.
  public static final int LYNX_ERROR_CODE_RELOAD_TEMPLATE = LynxErrorBehavior.EB_APP_BUNDLE_RELOAD;
  // Error in Java or Objc Logic.
  public static final int LYNX_ERROR_EXCEPTION = LynxErrorBehavior.EB_EXCEPTION_PLATFORM;
}
