// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXCONTEXT_H_
#define DARWIN_COMMON_LYNX_LYNXCONTEXT_H_

#import <Foundation/Foundation.h>
#import <Lynx/JSModule.h>
#import <Lynx/LynxError.h>
#import <Lynx/LynxView.h>

@protocol LynxExtensionModule;

NS_ASSUME_NONNULL_BEGIN

FOUNDATION_EXPORT NSString *const kDefaultComponentID;

@interface LynxContext : NSObject

- (void)sendGlobalEvent:(nonnull NSString *)name withParams:(nullable NSArray *)params;
- (nullable JSModule *)getJSModule:(nonnull NSString *)name;
- (nullable NSNumber *)getLynxRuntimeId;

- (void)reportModuleCustomError:(NSString *)message;
- (void)reportLynxError:(LynxError *)error;
- (nullable LynxView *)getLynxView;

- (void)runOnTasmThread:(dispatch_block_t)task;
- (void)runOnJSThread:(dispatch_block_t)task;
- (void)runOnLayoutThread:(dispatch_block_t)task;

// Experimental method. Must be called on Main Thread.
- (void)setExtensionModule:(nonnull id<LynxExtensionModule>)extensionModule
                    forKey:(nonnull NSString *)key;
- (nonnull id<LynxExtensionModule>)getExtensionModuleByKey:(nonnull NSString *)key;

// Experimental: Temporary flags for genericResoruceRefecher verifying  (will be removed)
@property(nonatomic, assign) BOOL hasCustomGenericFetcher;
@property(nonatomic, assign) BOOL hasCustomMediaFetcher;
@property(nonatomic, assign) BOOL hasCustomTemplateFetcher;
@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_LYNXCONTEXT_H_
