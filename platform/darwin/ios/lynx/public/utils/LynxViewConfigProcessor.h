// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxView.h>

NS_ASSUME_NONNULL_BEGIN

static NSString *const KEY_LYNX_ENABLE_MULTI_ASYNC_THREAD = @"multi_async_thread";
static NSString *const KEY_LYNX_PLATFORM_CONFIG = @"platform_config";
static NSString *const KEY_LYNX_RUNTIME_TYPE = @"runtime_type";
static NSString *const KEY_LYNX_ENABLE_BYTECODE = @"enable_bytecode";
static NSString *const KEY_LYNX_BYTECODE_URL = @"bytecode_url";

@interface LynxViewConfigProcessor : NSObject

+ (void)processorMap:(NSMutableDictionary *)dictionary
     lynxViewBuilder:(LynxViewBuilder *)lynxViewBuilder;

@end

NS_ASSUME_NONNULL_END
