// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_RESOURCE_LYNXTEMPLATERESOURCE_H_
#define DARWIN_COMMON_LYNX_RESOURCE_LYNXTEMPLATERESOURCE_H_

#import "LynxTemplateBundle.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * represents the result of template resource, it may be a NSData* or LynxTemplateBundle*
 */
@interface LynxTemplateResource : NSObject

@property(nonatomic, readonly, strong) NSData* _Nonnull data;

@property(nonatomic, readonly, strong) LynxTemplateBundle* _Nonnull bundle;

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithNSData:(NSData* _Nonnull)data;

- (instancetype)initWithBundle:(LynxTemplateBundle* _Nonnull)bundle;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_RESOURCE_LYNXTEMPLATERESOURCE_H_
