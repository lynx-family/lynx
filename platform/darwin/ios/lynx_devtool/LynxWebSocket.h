// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxWebSocketModule.h>
#import <SocketRocket/SRWebSocket.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxWebSocket : NSObject <WebSocketProtocol, SRWebSocketDelegate>

- (void)connect:(NSString *)urlString
      protocols:(NSArray *)protocols
        options:(WebSocketConnectOptions *)options
       socketID:(double)socketID;

- (void)send:(NSString *)message forSocketID:(double)socketID;

- (void)sendBinary:(NSString *)base64String forSocketID:(double)socketID;

- (void)sendData:(NSData *)data forSocketID:(NSNumber *__nonnull)socketID;

- (void)ping:(double)socketID;

- (void)close:(double)code reason:(NSString *)reason socketID:(double)socketID;

- (instancetype)initWithLynxContext:(LynxContext *)context;

@end

NS_ASSUME_NONNULL_END
