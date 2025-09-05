// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_PLATFORM_DARWIN_LYNXBASEENV_H_
#define BASE_PLATFORM_DARWIN_LYNXBASEENV_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxBaseEnv : NSObject

+ (instancetype)sharedInstance;

- (bool)initialize:(bool)print_logs_to_all_channels;

@end

NS_ASSUME_NONNULL_END

#endif  // BASE_PLATFORM_DARWIN_LYNXBASEENV_H_
