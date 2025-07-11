// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxFetchModuleEventSender : NSObject
@property(nonatomic, weak) id eventSender;
- (void)sendGlobalEvent:(NSString *)name withParams:(nullable NSArray *)params;
@end

@interface LynxHttpStreamingDelegate : NSObject
- (instancetype)initWithParam:(LynxFetchModuleEventSender *)sender
              withStreamingId:(NSString *)streamingId;
- (void)processChunkedData:(NSMutableData *)buffer withData:(NSData *)data;
- (void)onData:(NSData *)bytes;
- (void)onEnd;
- (void)onError:(NSString *)error;
@end

NS_ASSUME_NONNULL_END
