// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DARWIN_COMMON_LYNX_LYNXERRORCODE_H_
#define DARWIN_COMMON_LYNX_LYNXERRORCODE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN
// clang-format off

/**
 * ATTENTION !!!
 * This is a legacy error code file and will be deleted in the future.
 * Please use error behavior code in `LynxErrorBehavior.h` instead.
 */

// Error code for no error.
FOUNDATION_EXPORT NSInteger const LynxErrorCodeSuccess;

// Error occurred when loadTemplate, one should retry loadTemplate.
FOUNDATION_EXPORT NSInteger const LynxErrorCodeLoadTemplate;
// Error occurred when fetch template resource.
FOUNDATION_EXPORT NSInteger const LynxErrorCodeTemplateProvider;
// ReloadTemplate before loadTemplate, one should loadTemplate firstly.
FOUNDATION_EXPORT NSInteger const LynxErrorCodeReloadTemplate;

// Error occurred when executing JavaScript code.
FOUNDATION_EXPORT NSInteger const LynxErrorCodeJavaScript;
// Error occurred when fetch resource. Check error message for more information.
FOUNDATION_EXPORT NSInteger const LynxErrorCodeForResourceError;
// Error occurred when Layout, one should retry or fallback
FOUNDATION_EXPORT NSInteger const LynxErrorCodeLayout;
// Some unknown error in Update data pipeline. Check error message for more information.
FOUNDATION_EXPORT NSInteger const LynxErrorCodeUpdate;
// Error in Java or Objc Logic.
FOUNDATION_EXPORT NSInteger const LynxErrorCodeException;

NS_ASSUME_NONNULL_END

// clang-format on

#endif  // DARWIN_COMMON_LYNX_LYNXERRORCODE_H_
