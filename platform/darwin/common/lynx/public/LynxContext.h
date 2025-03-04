// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXCONTEXT_H_
#define DARWIN_COMMON_LYNX_LYNXCONTEXT_H_

#import <Foundation/Foundation.h>
#import "JSModule.h"
#import "LynxView.h"
@protocol LynxExtensionModule;

NS_ASSUME_NONNULL_BEGIN

FOUNDATION_EXPORT NSString *const kDefaultComponentID;

@interface LynxContext : NSObject

- (void)sendGlobalEvent:(nonnull NSString *)name withParams:(nullable NSArray *)params;
- (nullable JSModule *)getJSModule:(nonnull NSString *)name;
- (nullable NSNumber *)getLynxRuntimeId;

- (void)reportModuleCustomError:(NSString *)message;
- (nullable LynxView *)getLynxView;

- (void)runOnTasmThread:(dispatch_block_t)task;
- (void)runOnJSThread:(dispatch_block_t)task;

// Experimental method. Must be called on Main Thread.
- (void)setExtensionModule:(nonnull id<LynxExtensionModule>)extensionModule
                    forKey:(nonnull NSString *)key;
- (nonnull id<LynxExtensionModule>)getExtensionModuleByKey:(nonnull NSString *)key;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_LYNXCONTEXT_H_
