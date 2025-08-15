// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef DEVTOOL_BASE_DEVTOOL_DARWIN_IOS_LOGBOX_DEVTOOLLOGBOXPROXY_H_
#define DEVTOOL_BASE_DEVTOOL_DARWIN_IOS_LOGBOX_DEVTOOLLOGBOXPROXY_H_

#import <Foundation/Foundation.h>
#import "DevToolLogBoxResProvider.h"

NS_ASSUME_NONNULL_BEGIN

@interface DevToolLogBoxProxy : NSObject

@property(nonatomic, readwrite, nullable)
    NSMutableDictionary<NSString *, NSMutableArray *> *logMessages;  // level -> msg
@property(nullable, copy, nonatomic, readonly) NSDictionary *logSources;
@property(nullable, copy, nonatomic, readonly) NSString *entryUrlForLogSrc;

- (instancetype)initWithNamespace:(NSString *)errNamespace
                 resourceProvider:(id<DevToolLogBoxResProvider>)provider;
- (void)registerErrorParserWithBundle:(NSString *)bundleUrl file:(NSString *)filePath;
- (nullable NSMutableArray *)logMessagesWithLevel:(NSString *)level;
- (void)removeLogMessagesWithLevel:(NSString *)level;
- (void)showLogMessage:(NSString *)error withLevel:(NSString *)level;
- (NSString *)getErrorNamespace;
- (void)onMovedToWindow;
- (void)onResourceProviderReady;
- (NSString *)logSourceWithFileName:(NSString *)fileName;
- (void)reset;
- (void)destroy;

@end

NS_ASSUME_NONNULL_END

#endif  // DEVTOOL_BASE_DEVTOOL_DARWIN_IOS_LOGBOX_DEVTOOLLOGBOXPROXY_H_
