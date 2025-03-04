// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxBoxShadowManager.h"
#import "LynxConverter.h"

NS_ASSUME_NONNULL_BEGIN
@class LynxUI;
@interface LynxConverter (NSShadow)
+ (NSShadow *)toNSShadow:(NSArray<LynxBoxShadow *> *)shadowArr;
@end

NS_ASSUME_NONNULL_END
