// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxDebugBridge : NSObject

@property(readwrite, nonatomic) NSDictionary *hostOptions;
@property(readwrite, nonatomic) NSString *monitorWindowUrl;
@property(readwrite, nonatomic) NSString *debugState;

// this will be used in .m(objective-c). so this statement can't convert to c++ using expression.
typedef void (^LynxDebugBridgeOpenCardCallback)(NSString *);

+ (instancetype)singleton;

- (BOOL)isEnabled;
- (BOOL)hasSetOpenCardCallback;
- (BOOL)enable:(NSURL *)url withOptions:(NSDictionary *)options;
- (void)sendDebugStateEvent;
- (void)setOpenCardCallback:(LynxDebugBridgeOpenCardCallback)callback;
- (void)openCard:(NSString *)url;
- (void)onMessage:(NSString *)message withType:(NSString *)type;
- (void)setAppInfo:(NSDictionary *)hostOptions;
- (void)onPerfMetricsEvent:(NSString *_Nonnull)eventName
                  withData:(NSDictionary<NSString *, NSObject *> *_Nonnull)data
                instanceId:(int32_t)instanceId;

@end

NS_ASSUME_NONNULL_END
