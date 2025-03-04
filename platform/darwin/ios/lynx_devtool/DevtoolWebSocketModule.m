// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "DevToolWebSocketModule.h"
#import <Lynx/LynxBaseInspectorOwnerNG.h>
#import <Lynx/LynxView.h>

@interface DevToolWebSocketMessageHandler : NSObject
@end
@interface DevToolWebSocketMessageHandler () <MessageHandler>
@end
@implementation DevToolWebSocketMessageHandler {
  __weak LynxContext *context_;
  NSString *eventName_;
}

- (instancetype)initWithEventName:(NSString *)eventName context:(LynxContext *)context {
  if (self = [super init]) {
    context_ = context;
    eventName_ = eventName;
  }
  return self;
}

- (void)onMessage:(NSString *)message {
  __strong typeof(context_) context = context_;
  if (!context) {
    return;
  }
  NSMutableArray *args = [[NSMutableArray alloc] init];
  [args addObject:message];
  [context sendGlobalEvent:eventName_ withParams:args];
}

@end

@interface DevtoolWebSocketModule ()

@end

@implementation DevtoolWebSocketModule {
  __weak LynxContext *context_;
  DevToolWebSocketMessageHandler *remoteCallHandler_;
  DevToolWebSocketMessageHandler *reactCallHandler_;
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  self = [super init];
  if (self) {
    context_ = context;
    id<LynxBaseInspectorOwner> owner = [self getInspectorOwner];
    if (owner != nil && [owner conformsToProtocol:@protocol(LynxBaseInspectorOwnerNG)]) {
      remoteCallHandler_ =
          [[DevToolWebSocketMessageHandler alloc] initWithEventName:@"OnRemoteCallMessage"
                                                            context:context];
      [(id<LynxBaseInspectorOwnerNG>)owner subscribeMessage:@"RemoteCall"
                                                withHandler:remoteCallHandler_];
      reactCallHandler_ =
          [[DevToolWebSocketMessageHandler alloc] initWithEventName:@"OnReactDevtoolMessage"
                                                            context:context];
      [(id<LynxBaseInspectorOwnerNG>)owner subscribeMessage:@"ReactDevtool"
                                                withHandler:reactCallHandler_];
    }
  }

  return self;
}

+ (NSString *)name {
  return @"DevtoolWebSocketModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"postMessage" : NSStringFromSelector(@selector(postMessage:type:)),
    @"postMessageWithMark" : NSStringFromSelector(@selector(postMessage:type:mark:)),
  };
}

- (void)postMessage:(NSString *)message type:(NSString *)type mark:(int)mark {
  __strong typeof(context_) context = context_;
  if (!context) {
    return;
  }
  id<LynxBaseInspectorOwner> owner = [self getInspectorOwner];
  if (owner != nil && [owner conformsToProtocol:@protocol(LynxBaseInspectorOwnerNG)]) {
    CustomizedMessage *msg = [[CustomizedMessage alloc] init];
    msg.type = type;
    msg.data = message;
    msg.mark = mark;
    [(id<LynxBaseInspectorOwnerNG>)owner sendMessage:msg];
  }
}

- (void)postMessage:(NSString *)message type:(NSString *)type {
  [self postMessage:message type:type mark:-1];
}

- (void)onMessage:(NSString *)message {
  __strong typeof(context_) context = context_;
  if (!context) {
    return;
  }
  NSMutableArray *args = [[NSMutableArray alloc] init];
  [args addObject:message];
  [context sendGlobalEvent:@"onRemoteCallMessage" withParams:args];
}

- (nullable id<LynxBaseInspectorOwner>)getInspectorOwner {
  id<LynxBaseInspectorOwner> owner = nil;
  __strong typeof(context_) context = context_;
  LynxView *_lynxView = [context getLynxView];
  if (_lynxView != nil) {
    owner = _lynxView.baseInspectorOwner;
  }
  return owner;
}

@end
