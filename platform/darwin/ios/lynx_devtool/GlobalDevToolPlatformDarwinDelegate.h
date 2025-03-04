// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxView+Internal.h>
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

NS_ASSUME_NONNULL_BEGIN

@interface GlobalDevToolPlatformDarwinDelegate : NSObject

+ (void)startMemoryTracing;
+ (void)stopMemoryTracing;

+ (intptr_t)getTraceController;
+ (intptr_t)getFPSTracePlugin;
+ (intptr_t)getFrameViewTracePlugin;
+ (intptr_t)getInstanceTracePlugin;

+ (std::string)getSystemModelName;

@end

NS_ASSUME_NONNULL_END
