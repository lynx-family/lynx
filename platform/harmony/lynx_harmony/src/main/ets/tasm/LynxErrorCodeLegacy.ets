// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxErrorBehavior } from './gen/LynxErrorBehavior';
import { LynxSubErrorCode } from './gen/LynxSubErrorCode';

/**
 * @deprecated
 * `ATTENTION !!!`
 * This is a legacy error code file and will be deleted in the future.
 * Please use error behavior code in `LynxErrorBehavior` instead.
 */
export class LynxErrorCodeLegacy {
  // Error code for no error.
  public static readonly LYNX_ERROR_CODE_SUCCESS: number = 0;
  // Error occurred when loadTemplate, one should retry loadTemplate.
  public static readonly LYNX_ERROR_CODE_LOAD_TEMPLATE: number = LynxErrorBehavior.EB_APP_BUNDLE_LOAD;
  // Error occurred when fetch template resource.
  public static readonly LYNX_ERROR_CODE_TEMPLATE_PROVIDER: number = LynxErrorBehavior.EB_APP_BUNDLE_LOAD;
  // Error occurred when executing JavaScript code.
  public static readonly LYNX_ERROR_CODE_JAVASCRIPT: number = LynxErrorBehavior.EB_BTS_RUNTIME_ERROR;
  // Resource related errors
  public static readonly LYNX_ERROR_CODE_RESOURCE: number = LynxErrorBehavior.EB_RESOURCE_IMAGE;
  // Error from image resource provider
  public static readonly LYNX_ERROR_CODE_PIC_SOURCE: number = LynxSubErrorCode.E_RESOURCE_IMAGE_PIC_SOURCE;
  // Network or other issues
  public static readonly LYNX_ERROR_CODE_FROM_T_T_NET_OR_OTHERS: number =
    LynxSubErrorCode.E_RESOURCE_IMAGE_FROM_NETWORK_OR_OTHERS;
  // Failed when load a dynamic component.
  public static readonly LYNX_ERROR_DYNAMIC_COMPONENT_LOAD_FAIL: number = LynxErrorBehavior.EB_LAZY_BUNDLE_LOAD;
}