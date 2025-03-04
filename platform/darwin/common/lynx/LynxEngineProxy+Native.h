// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_ENGINE_PROXY_NATIVE_H_
#define DARWIN_COMMON_LYNX_ENGINE_PROXY_NATIVE_H_

#import <Foundation/Foundation.h>
#import "LynxEngineProxy.h"

#include "core/shell/ios/lynx_engine_proxy_darwin.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxEngineProxy ()

/**
 * Set the shared pointer of lynx::shell::LynxEngineProxyDarwin
 */
- (void)setNativeEngineProxy:(std::shared_ptr<lynx::shell::LynxEngineProxyDarwin>)proxy;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_ENGINE_PROXY_NATIVE_H_
