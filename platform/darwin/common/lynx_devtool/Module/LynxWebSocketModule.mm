// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 MIT License

 Copyright (c) Meta Platforms, Inc. and affiliates.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#import "LynxWebSocketModule.h"

#import <LynxWebSocket.h>

@interface LynxWebSocketModule ()

@end

@implementation LynxWebSocketModule {
  LynxWebSocket *webSocket_;
}

+ (NSString *)name {
  return @"LynxWebSocketModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"connect" : NSStringFromSelector(@selector(connect:protocols:options:socketID:)),
    @"send" : NSStringFromSelector(@selector(send:forSocketID:)),
    @"sendBinary" : NSStringFromSelector(@selector(sendBinary:forSocketID:)),
    @"ping" : NSStringFromSelector(@selector(ping:)),
    @"close" : NSStringFromSelector(@selector(close:reason:socketID:))
  };
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  self = [super init];
  if (self) {
    webSocket_ = [[LynxWebSocket alloc] initWithLynxContext:context];
  }
  return self;
}

- (void)connect:(NSString *)urlString
      protocols:(NSArray *)protocols
        options:(WebSocketConnectOptions *)options
       socketID:(double)socketID {
  [webSocket_ connect:urlString protocols:protocols options:options socketID:socketID];
}

- (void)send:(NSString *)message forSocketID:(double)socketID {
  [webSocket_ send:message forSocketID:socketID];
}

- (void)sendBinary:(NSString *)base64String forSocketID:(double)socketID {
  [webSocket_ sendBinary:base64String forSocketID:socketID];
}

- (void)sendData:(NSData *)data forSocketID:(NSNumber *__nonnull)socketID {
  [webSocket_ sendData:data forSocketID:socketID];
}

- (void)ping:(double)socketID {
  [webSocket_ ping:socketID];
}

- (void)close:(double)code reason:(NSString *)reason socketID:(double)socketID {
  [webSocket_ close:code reason:reason socketID:socketID];
}

@end
