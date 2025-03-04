// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// clang-format off
#import "LynxErrorCode.h"

/**
 * ATTENTION !!!
 * This is a legacy error code file and will be deleted in the future.
 * Please use error behavior code in `LynxErrorBehavior.h` instead.
 */

// Error code for no error.
NSInteger const LynxErrorCodeSuccess = 0;

// Error occurred when loadTemplate, one should retry loadTemplate.
NSInteger const LynxErrorCodeLoadTemplate = 102;
// Error occurred when fetch template resource.
NSInteger const LynxErrorCodeTemplateProvider = 103;

// Error occurred when executing JavaScript code.
NSInteger const LynxErrorCodeJavaScript = 201;

NSInteger const LynxErrorCodeForResourceError = 301;

NSInteger const LynxErrorCodeLayout = 699;

NSInteger const LynxErrorCodeUpdate = 401;

NSInteger const LynxErrorCodeReloadTemplate = 105;

NSInteger const LynxErrorCodeException = 9901;

// clang-format on
