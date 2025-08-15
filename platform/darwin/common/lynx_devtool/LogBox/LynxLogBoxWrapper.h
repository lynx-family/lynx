// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef LYNX_PLATFORM_DARWIN_COMMON_LYNX_DEVTOOL_LOG_BOX_LYNX_LOG_BOX_WRAPPER_H_
#define LYNX_PLATFORM_DARWIN_COMMON_LYNX_DEVTOOL_LOG_BOX_LYNX_LOG_BOX_WRAPPER_H_

#if OS_OSX
#import <Lynx/DevToolLogBoxResProvider.h>
#else
#import <BaseDevtool/DevToolLogBoxResProvider.h>
#endif
#import <Foundation/Foundation.h>
#import <Lynx/LynxLogBoxProtocol.h>
#import <Lynx/LynxView.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxLogBoxWrapper : NSObject <LynxLogBoxProtocol, DevToolLogBoxResProvider>

- (instancetype)initWithLynxView:(nullable LynxView *)view;

@end

NS_ASSUME_NONNULL_END

#endif  // LYNX_PLATFORM_DARWIN_COMMON_LYNX_DEVTOOL_LOG_BOX_LYNX_LOG_BOX_WRAPPER_H_
