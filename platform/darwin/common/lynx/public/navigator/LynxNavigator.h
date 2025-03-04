// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_NAVIGATOR_LYNXNAVIGATOR_H_
#define DARWIN_COMMON_LYNX_NAVIGATOR_LYNXNAVIGATOR_H_

#import <Foundation/Foundation.h>
#import "LynxHolder.h"
#import "LynxRoute.h"
#import "LynxSchemaInterceptor.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxNavigator : NSObject

@property(nonatomic, readonly) NSInteger capacity;
@property(nonatomic) id<LynxSchemaInterceptor> interceptor;

+ (instancetype)sharedInstance;

- (void)setCapacity:(NSInteger)capacity;
- (void)setSchemaInterceptor:(id<LynxSchemaInterceptor>)interceptor;

/*
 Called to register/unregister ViewController;
 */
- (void)registerLynxHolder:(id<LynxHolder>)lynxHolder;
- (void)registerLynxHolder:(id<LynxHolder>)lynxHolder initLynxView:(nullable LynxView *)lynxView;
- (void)unregisterLynxHolder:(id<LynxHolder>)lynxHolder;

/*
 Called by NativeModule;
 */
- (void)navigate:(nonnull NSString *)name withParam:(nonnull NSDictionary *)param;
- (void)replace:(nonnull NSString *)name withParam:(nonnull NSDictionary *)param;
- (void)goBack;

/*
 Called When ViewController swipe back;
 */
- (BOOL)onNavigateBack;

- (void)didEnterForeground:(id<LynxHolder>)lynxHolder;

- (void)didEnterBackground:(id<LynxHolder>)lynxHolder;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_NAVIGATOR_LYNXNAVIGATOR_H_
