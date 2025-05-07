// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_DARWIN_COMMON_LYNX_DEVTOOL_LOGBOX_DEVTOOL_LOGBOX_ENV_H_
#define PLATFORM_DARWIN_COMMON_LYNX_DEVTOOL_LOGBOX_DEVTOOL_LOGBOX_ENV_H_

#import "LogBoxFileLoadUtils.h"

typedef void (^LoadErrorParserBlock)(LogBoxFileLoadCallback completion);

NS_ASSUME_NONNULL_BEGIN

@protocol LogBoxErrorParserLoaderProtocol <NSObject>
- (void)loadErrorParserWithCallback:(LogBoxFileLoadCallback)completion;
@end

@interface DevToolLogBoxEnv : NSObject
+ (DevToolLogBoxEnv *)sharedInstance;
- (void)registerErrorParserLoader:(LoadErrorParserBlock)loadBlock
                    withNamespace:(NSString *)errNamespace;
- (void)loadErrorParser:(NSString *)errNamespace completion:(LogBoxFileLoadCallback)completion;
@end

NS_ASSUME_NONNULL_END

#endif  // PLATFORM_DARWIN_COMMON_LYNX_DEVTOOL_LOGBOX_DEVTOOL_LOGBOX_ENV_H_
