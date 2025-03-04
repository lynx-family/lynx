// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_UTILS_LYNXCONVERTUTILS_H_
#define DARWIN_COMMON_LYNX_UTILS_LYNXCONVERTUTILS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxConvertUtils : NSObject

+ (NSString *)convertToJsonData:(NSDictionary *)dict;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_UTILS_LYNXCONVERTUTILS_H_
