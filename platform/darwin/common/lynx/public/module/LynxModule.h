// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_MODULE_LYNXMODULE_H_
#define DARWIN_COMMON_LYNX_MODULE_LYNXMODULE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^LynxCallbackBlock)(id result);
typedef void (^LynxPromiseResolveBlock)(id result);
typedef void (^LynxPromiseRejectBlock)(NSString *code, NSString *message);
typedef BOOL (^LynxMethodBlock)(NSString *method, NSString *module, NSString *invoke_session,
                                NSInvocation *inv);
typedef NSDictionary *_Nullable (^LynxMethodSessionBlock)(NSString *method, NSString *module,
                                                          NSString *invoke_session,
                                                          NSString *name_space);

@protocol LynxModule <NSObject>

/*! Module Name. */
@property(nonatomic, readonly, copy, class) NSString *name;

/*! Module methods look up table. The keys are JS method names, while values are Objective C
selectors.
///    - (NSDictionary<NSString *,NSString *> *)methodLookup {
///      return @{
///        @"voidFunc" : NSStringFromSelector(@selector(voidFunc)),
///        @"getNumber" : NSStringFromSelector(@selector(getNumber:)),
///      };
///    } */
@property(nonatomic, readonly, copy, class) NSDictionary<NSString *, NSString *> *methodLookup;

@optional
@property(nonatomic, readonly, copy, class) NSDictionary *attributeLookup;

@optional
@property(nonatomic, nullable) id extraData;

@optional
- (instancetype)init;
- (instancetype)initWithParam:(id)param;
- (void)destroy;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_MODULE_LYNXMODULE_H_
