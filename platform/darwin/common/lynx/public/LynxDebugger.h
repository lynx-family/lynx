// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXDEBUGGER_H_
#define DARWIN_COMMON_LYNX_LYNXDEBUGGER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^LynxOpenCardCallback)(NSString *);

@protocol LynxDebuggerProtocol <NSObject>

@required

+ (instancetype)singleton;

- (BOOL)enable:(NSURL *)url withOptions:(NSDictionary *)options;

- (void)setOpenCardCallback:(LynxOpenCardCallback)callback;

@end

@interface LynxDebugger : NSObject

+ (BOOL)enable:(NSURL *)schema withOptions:(NSDictionary *)options;

+ (void)setOpenCardCallback:(LynxOpenCardCallback)callback
    __attribute__((deprecated("Use addOpenCardCallback instead after lynx 2.6")));
;

+ (void)addOpenCardCallback:(LynxOpenCardCallback)callback;

+ (BOOL)hasSetOpenCardCallback;

// only be used by macOS
+ (BOOL)openDebugSettingPanel;

+ (void)onPerfMetricsEvent:(NSString *_Nonnull)eventName
                  withData:(NSDictionary<NSString *, NSObject *> *_Nonnull)data
                instanceId:(int32_t)instanceId;
@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_LYNXDEBUGGER_H_
