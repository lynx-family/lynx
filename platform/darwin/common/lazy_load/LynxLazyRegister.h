// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LAZYLOAD_LYNXLAZYREGISTER_H_
#define DARWIN_COMMON_LAZYLOAD_LYNXLAZYREGISTER_H_

#import <Foundation/Foundation.h>
#import "LynxLazyLoad.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxLazyRegister : NSObject

+ (void)loadLynxInitTask;
- (void)startTasksForKey:(NSString *)key;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LAZYLOAD_LYNXLAZYREGISTER_H_
