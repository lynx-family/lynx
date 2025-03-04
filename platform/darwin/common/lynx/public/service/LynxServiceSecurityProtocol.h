// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICESECURITYPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICESECURITYPROTOCOL_H_

#import "LynxServiceProtocol.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * Verify Result of SecurityService
 * verified: true if passed, else false;
 * errorMsg: reason for verify failed, nil if success;
 */
@interface LynxVerificationResult : NSObject
@property(nonatomic, assign) BOOL verified;
@property(nonatomic, copy, nullable) NSString* errorMsg;
@end

typedef NS_ENUM(NSInteger, LynxTASMType) {
  LynxTASMTypeTemplate = 0,
  LynxTASMTypeDynamicComponent = 1 << 0,
};

@class LynxView;
@protocol LynxServiceSecurityProtocol <LynxServiceProtocol>

/**
 * use specified verify logic to check the template consistency.
 *
 * template: the input tasm binary
 * @return result of the verification
 */
- (LynxVerificationResult*)verifyTASM:(NSData*)data
                                 view:(nullable LynxView*)lynxView
                                  url:(nullable NSString*)url
                                 type:(LynxTASMType)type;

/**
 * Invoked while PiperInvoked.
 * module: module name of piper;
 *                  method: method name of piper
 *                  param: coming params of piper
 *                  url: identity of the lynx template.
 */
- (BOOL)onPiperInvoked:(NSString*)module
                method:(NSString*)method
                 param:(NSString*)param
                   url:(NSString*)url;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICESECURITYPROTOCOL_H_
