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

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxWebSocket.h"

#import <objc/runtime.h>

@implementation SRWebSocket (Lynx)

- (NSNumber *)lynxTag {
  return objc_getAssociatedObject(self, _cmd);
}

- (void)setLynxTag:(NSNumber *)lynxTag {
  objc_setAssociatedObject(self, @selector(lynxTag), lynxTag, OBJC_ASSOCIATION_COPY_NONATOMIC);
}

@end

@implementation LynxWebSocket {
  __weak LynxContext *context_;

  NSMutableDictionary<NSNumber *, SRWebSocket *> *sockets_;
}

- (nonnull instancetype)initWithLynxContext:(nonnull LynxContext *)context {
  context_ = context;
  return self;
}

- (dispatch_queue_t)methodQueue {
  return dispatch_get_main_queue();
}

- (void)connect:(NSString *)urlString
      protocols:(NSArray *)protocols
        options:(WebSocketConnectOptions *)options
       socketID:(double)socketID {
  NSURL *URL = [NSURL URLWithString:urlString];
  NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:URL];

  // We load cookies from sharedHTTPCookieStorage (shared with XHR and
  // fetch). To get secure cookies for wss URLs, replace wss with https
  // in the URL.
  NSURLComponents *components = [NSURLComponents componentsWithURL:URL
                                           resolvingAgainstBaseURL:true];
  if ([components.scheme.lowercaseString isEqualToString:@"wss"]) {
    components.scheme = @"https";
  }

  // Load and set the cookie header.
  NSArray<NSHTTPCookie *> *cookies =
      [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:components.URL];
  request.allHTTPHeaderFields = [NSHTTPCookie requestHeaderFieldsWithCookies:cookies];

  SRWebSocket *webSocket = [[SRWebSocket alloc] initWithURLRequest:request protocols:protocols];
  [webSocket setDelegateDispatchQueue:[self methodQueue]];
  webSocket.delegate = self;
  webSocket.lynxTag = @(socketID);
  if (!sockets_) {
    sockets_ = [NSMutableDictionary new];
  }
  sockets_[@(socketID)] = webSocket;
  [webSocket open];
}

- (void)close:(double)code reason:(NSString *)reason socketID:(double)socketID {
  [sockets_[@(socketID)] closeWithCode:code reason:reason];
  [sockets_ removeObjectForKey:@(socketID)];
}

- (void)ping:(double)socketID {
  [sockets_[@(socketID)] sendPing:nil error:nil];
}

- (void)send:(NSString *)message forSocketID:(double)socketID {
  [sockets_[@(socketID)] sendString:message error:nil];
}

- (void)sendBinary:(NSString *)base64String forSocketID:(double)socketID {
  [self sendData:[[NSData alloc] initWithBase64EncodedString:base64String options:0]
      forSocketID:@(socketID)];
}

- (void)sendData:(NSData *)data forSocketID:(NSNumber *__nonnull)socketID {
  [sockets_[socketID] sendData:data error:nil];
}

#pragma mark - SRWebSocketDelegate methods

- (void)webSocket:(SRWebSocket *)webSocket didReceiveMessage:(id)message {
  __strong typeof(context_) context = context_;
  if (!context) {
    return;
  }

  NSString *type;

  if ([message isKindOfClass:[NSData class]]) {
    type = @"binary";
    message = [message base64EncodedStringWithOptions:0];
  } else {
    type = @"text";
  }

  [context sendGlobalEvent:@"websocketMessage"
                withParams:@[ @{@"data" : message, @"type" : type, @"id" : webSocket.lynxTag} ]];
}

- (void)webSocketDidOpen:(SRWebSocket *)webSocket {
  __strong typeof(context_) context = context_;
  if (!context) {
    return;
  }
  [context sendGlobalEvent:@"websocketOpen"
                withParams:@[ @{
                  @"id" : webSocket.lynxTag,
                  @"protocol" : webSocket.protocol ? webSocket.protocol : @""
                } ]];
}

- (void)webSocket:(SRWebSocket *)webSocket didFailWithError:(NSError *)error {
  __strong typeof(context_) context = context_;
  if (!context) {
    return;
  }
  NSNumber *socketID = [webSocket lynxTag];
  sockets_[socketID] = nil;
  NSDictionary *body = @{
    @"message" : error.localizedDescription ?: @"Undefined, error is nil",
    @"id" : socketID ?: @(-1)
  };
  [context sendGlobalEvent:@"websocketFailed" withParams:@[ body ]];
}

- (void)webSocket:(SRWebSocket *)webSocket
    didCloseWithCode:(NSInteger)code
              reason:(NSString *)reason
            wasClean:(BOOL)wasClean {
  __strong typeof(context_) context = context_;
  if (!context) {
    return;
  }
  NSNumber *socketID = [webSocket lynxTag];
  sockets_[socketID] = nil;
  [context sendGlobalEvent:@"websocketClosed"
                withParams:@[ @{
                  @"code" : @(code),
                  @"reason" : (reason ?: (id)kCFNull),
                  @"clean" : @(wasClean),
                  @"id" : socketID
                } ]];
}

@end
