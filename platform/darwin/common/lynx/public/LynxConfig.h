// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxComponentRegistry.h"
#import "LynxModule.h"
#import "LynxTemplateProvider.h"

NS_ASSUME_NONNULL_BEGIN

/*!
 Config some common tools that may be used in the lifecycle of LynxView.
 LynxConfig can be reused for multiple LynxViews.
 */
@interface LynxConfig : NSObject

@property(nonatomic, readonly) id<LynxTemplateProvider> templateProvider;
@property(nonatomic) LynxComponentScopeRegistry *componentRegistry;
@property(nonatomic, copy, nullable) NSMutableDictionary *contextDict;

/*! Set a global (default) config which will provide a convenient way
 for creating LynxView without LynxConfig. */
+ (LynxConfig *)globalConfig
    __attribute__((deprecated("Use [LynxEnv sharedInstance].config instead.")));
+ (void)prepareGlobalConfig:(LynxConfig *)config
    __attribute__((deprecated("Use [[LynxEnv sharedInstance] prepareConfig:config] instead.")));

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithProvider:(nullable id<LynxTemplateProvider>)provider;
- (void)registerModule:(Class<LynxModule>)module;
- (void)registerModule:(Class<LynxModule>)module param:(nullable id)param;
- (void)registerUI:(Class)ui withName:(NSString *)name;
- (void)registerShadowNode:(Class)node withName:(NSString *)name;
- (void)registerMethodAuth:(LynxMethodBlock)authBlock;
- (void)registerContext:(NSDictionary *)ctxDict sessionInfo:(LynxMethodSessionBlock)sessionInfo;

@end

NS_ASSUME_NONNULL_END
