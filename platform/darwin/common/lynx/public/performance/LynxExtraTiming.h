// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_PERFORMANCE_LYNXEXTRATIMING_H_
#define DARWIN_COMMON_LYNX_PERFORMANCE_LYNXEXTRATIMING_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxExtraTiming : NSObject

@property(nonatomic, assign) uint64_t openTime;
@property(nonatomic, assign) uint64_t containerInitStart;
@property(nonatomic, assign) uint64_t containerInitEnd;
@property(nonatomic, assign) uint64_t prepareTemplateStart;
@property(nonatomic, assign) uint64_t prepareTemplateEnd;

- (NSDictionary *)toDictionary;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_PERFORMANCE_LYNXEXTRATIMING_H_
