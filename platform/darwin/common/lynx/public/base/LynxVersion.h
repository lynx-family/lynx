// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXVERSION_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXVERSION_H_

#import <Foundation/Foundation.h>

static const NSString* LYNX_TARGET_SDK_VERSION_1_5 = @"1.5";

@interface LynxVersion : NSObject

+ (NSString*)versionString;

@end

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXVERSION_H_
