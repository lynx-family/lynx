// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <AnimaX/AnimaXMonitorService.h>
#import <Foundation/Foundation.h>
#import <Lynx/LynxUIContext.h>

NS_ASSUME_NONNULL_BEGIN

// Lynx-specific implementation of AnimaXMonitorService
@interface LynxAnimaXMonitorServiceImpl : NSObject <AnimaXMonitorService>

// Designated initializer with context
- (instancetype)initWithLynxUIContext:(nullable LynxUIContext *)context;

@end

NS_ASSUME_NONNULL_END
