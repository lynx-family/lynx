// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DEVTOOL_BASE_DEVTOOL_DARWIN_IOS_LOGBOX_DEVTOOLLOGBOXENV_H_
#define DEVTOOL_BASE_DEVTOOL_DARWIN_IOS_LOGBOX_DEVTOOLLOGBOXENV_H_

#import "DevToolFileLoadUtils.h"

typedef void (^LoadErrorParserBlock)(DevToolFileLoadCallback _Nonnull completion);

NS_ASSUME_NONNULL_BEGIN

@protocol LogBoxErrorParserLoaderProtocol <NSObject>
- (void)loadErrorParserWithCallback:(DevToolFileLoadCallback)completion;
@end

@interface DevToolLogBoxEnv : NSObject
+ (DevToolLogBoxEnv *)sharedInstance;
- (void)registerErrorParserLoader:(LoadErrorParserBlock)loadBlock
                    withNamespace:(NSString *)errNamespace;
- (void)loadErrorParser:(NSString *)errNamespace completion:(DevToolFileLoadCallback)completion;
@end

NS_ASSUME_NONNULL_END

#endif  // DEVTOOL_BASE_DEVTOOL_DARWIN_IOS_LOGBOX_DEVTOOLLOGBOXENV_H_
