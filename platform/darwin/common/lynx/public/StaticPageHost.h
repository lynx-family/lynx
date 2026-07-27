// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_STATICPAGEHOST_H_
#define DARWIN_COMMON_LYNX_STATICPAGEHOST_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol StaticPageInstance <NSObject>
// The payloads are NSDictionary instances. They use id here so Swift receives
// the original Objective-C objects without importing them as Swift Dictionary.
- (void)renderPage:(id)data globalProps:(nullable id)globalProps;
- (void)updateMetaData:(nullable id)data globalProps:(nullable id)globalProps;
- (void)destroy;
@end

@interface StaticPageHost : NSObject

+ (BOOL)attach:(int32_t)instanceId
      instance:(id<StaticPageInstance>)instance NS_SWIFT_NAME(attach(_:instance:));

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_STATICPAGEHOST_H_
