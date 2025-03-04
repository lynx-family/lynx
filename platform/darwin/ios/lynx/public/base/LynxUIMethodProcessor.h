// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Foundation/Foundation.h>
#import "LynxDefines.h"
#import "LynxModule.h"

@class LynxUI;

NS_ASSUME_NONNULL_BEGIN

#define LYNX_UI_METHOD_CONFIG_PREFIX __lynx_ui_method_config__
#define LYNX_UI_METHOD_CONFIG_PREFIX_STR @"__lynx_ui_method_config__"

#define LYNX_UI_METHOD_CONFIG_METHOD \
  LYNX_CONCAT(LYNX_UI_METHOD_CONFIG_PREFIX, LYNX_CONCAT(__LINE__, __COUNTER__))

#define LYNX_UI_METHOD(method)                            \
  +(NSString*)LYNX_UI_METHOD_CONFIG_METHOD LYNX_DYNAMIC { \
    return @ #method;                                     \
  }                                                       \
  -(void)method : (NSDictionary*)params withResult        \
      : (LynxUIMethodCallbackBlock)callback LYNX_DYNAMIC

enum LynxUIMethodErrorCode {
  kUIMethodSuccess = 0,
  kUIMethodUnknown,
  kUIMethodNodeNotFound,
  kUIMethodMethodNotFound,
  kUIMethodParamInvalid,
  kUIMethodSelectorNotSupported,
  kUIMethodNoUiForNode,
  kUIMethodInvalidStateError,
  kUIMethodOperationError
};

typedef void (^LynxUIMethodCallbackBlock)(int code, id _Nullable data);

@interface LynxUIMethodProcessor : NSObject

+ (void)invokeMethod:(NSString*)method
          withParams:(NSDictionary*)params
          withResult:(LynxUIMethodCallbackBlock)callback
               forUI:(LynxUI*)ui;

@end

NS_ASSUME_NONNULL_END
