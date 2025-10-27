// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWGROUP_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWGROUP_H_

#import <Lynx/LynxBaseConfigurator.h>
#import <Lynx/LynxTemplateBundle.h>
#import <Lynx/LynxTemplateData.h>
#import <Lynx/LynxTemplateResourceFetcher.h>

NS_ASSUME_NONNULL_BEGIN

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

- (nullable LynxView *)getLynxViewById:(NSInteger)viewId;

@end

NS_ASSUME_NONNULL_END

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXBASECONFIGURATOR_H_
