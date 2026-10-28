// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_RESOURCE_LYNXTEMPLATERESOURCE_H_
#define DARWIN_COMMON_LYNX_RESOURCE_LYNXTEMPLATERESOURCE_H_

#import <Lynx/LynxResourceHandle.h>
#import <Lynx/LynxTemplateBundle.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * represents the result of template resource, it may be NSData*, LynxTemplateBundle*, or
 * LynxResourceHandle*
 */
@interface LynxTemplateResource : NSObject

@property(nonatomic, readonly, strong) NSData* _Nonnull data;

@property(nonatomic, readonly, strong) LynxTemplateBundle* _Nonnull bundle;

/**
 * A reusable resource handle containing the template source. The handle remains valid and reusable
 * after Lynx loads the template.
 */
@property(nonatomic, readonly, strong, nullable) LynxResourceHandle* resourceHandle;

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithNSData:(NSData* _Nonnull)data;

- (instancetype)initWithBundle:(LynxTemplateBundle* _Nonnull)bundle;

/**
 * Constructs a template resource from a reusable resource handle.
 */
- (instancetype)initWithResourceHandle:(LynxResourceHandle* _Nonnull)resourceHandle;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_RESOURCE_LYNXTEMPLATERESOURCE_H_
