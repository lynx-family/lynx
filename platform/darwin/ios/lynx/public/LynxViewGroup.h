// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWGROUP_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWGROUP_H_

#import <Lynx/LynxBaseConfigurator.h>
#import <Lynx/LynxModule.h>
#import <Lynx/LynxTemplateBundle.h>
#import <Lynx/LynxTemplateData.h>
#import <Lynx/LynxTemplateResourceFetcher.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^LynxTemplateBundleResultBlock)(LynxTemplateBundle *_Nullable data,
                                              NSError *_Nullable error);

@protocol LynxLogicExecutor;
@class LynxView;

@interface LynxViewGroup : LynxBaseConfigurator

/**
 * Url of the AppBundle associated with lynxViewGroup;
 */
@property(nonatomic, strong, nullable) NSString *url;

/**
 * GlobalProps of the lynxViewGroup
 */
@property(nonatomic, strong, nullable) LynxTemplateData *globalProps;

/**
 * Get the associated TemplateBundle with LynxViewGroup.
 * If templateBundle is not ready yet. It will block current thread
 * and waiting for the result.
 *
 * @return The Associated TemplateBundle
 */
@property(nonatomic, nullable, readonly) LynxTemplateBundle *templateBundle;

#pragma mark - Init

- (instancetype)initWithUrl:(NSString *)url
            templateFetcher:(id<LynxTemplateResourceFetcher>)templateFetcher;

- (instancetype)initWithUrl:(NSString *)url templateBundle:(LynxTemplateBundle *)bundle;

#pragma mark - Info

- (bool)isTemplateBundleReady;

- (void)destroyForInstance:(NSString *)instanceId;

/**
 * default logicExecutor
 */
@property(nonatomic, strong, nullable) id<LynxLogicExecutor> logicExecutor;

- (int)generateNextLynxViewID;
- (void)addLynxView:(int)lynxViewId view:(LynxView *_Nonnull)view;
- (void)removeLynxView:(int)lynxViewId;
- (LynxView *_Nullable)getLynxViewById:(int)lynxViewId;

- (void)setTemplateBundle:(LynxTemplateBundle *_Nullable)templateBundle;

- (void)setLogicExecutor:(id<LynxLogicExecutor>)logicExecutor;

/**
 * - If the TemplateBundle is ready, immediately invoke the callback (in-place)
 * - If the TemplateBundle is being fetched, trigger the callback once the fetch completes
 * - If the TemplateBundle failed to fetch, pass the failure error to the callback
 */
- (void)fetchTemplate:(LynxTemplateBundleResultBlock)callback;

/**
 * registerModule in lynxViewGroup
 */
- (void)registerModule:(Class<LynxModule>)module;
- (void)registerModule:(Class<LynxModule>)module param:(nullable id)param;
@end

NS_ASSUME_NONNULL_END

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXBASECONFIGURATOR_H_
