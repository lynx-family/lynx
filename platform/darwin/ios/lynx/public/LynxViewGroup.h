// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWGROUP_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWGROUP_H_

#import <Lynx/LynxBaseConfigurator.h>
#import <Lynx/LynxTemplateBundle.h>
#import <Lynx/LynxTemplateData.h>

@interface LynxViewGroup : LynxBaseConfigurator

/**
 * Url of the AppBundle associated with lynxViewGroup;
 */
@property(nonatomic, strong, nullable) NSString* url;

/**
 * Template bundle object associated with lynxViewGroup, the loading process
 * will use bundle in priority
 */
@property(nonatomic, strong, nullable) LynxTemplateBundle* templateBundle;

/**
 * GlobalProps of the lynxViewGroup
 */
@property(nonatomic, strong, nullable) LynxTemplateData* globalProps;

@end

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXBASECONFIGURATOR_H_
