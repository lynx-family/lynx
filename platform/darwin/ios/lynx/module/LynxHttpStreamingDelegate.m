// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBackgroundRuntime.h>
#import <Lynx/LynxBaseInspectorController.h>
#import <Lynx/LynxContext.h>
#import <Lynx/LynxDevtool.h>
#import <Lynx/LynxHttpStreamingDelegate.h>
#import <Lynx/LynxView+Internal.h>
#import "LynxFetchModule.h"

@interface LynxBackgroundRuntime (LynxFetchModuleInternal)
- (LynxDevtool *)devtool;
@end

@implementation LynxFetchModuleEventSender
- (void)sendGlobalEvent:(nonnull NSString *)name withParams:(nullable NSArray *)params {
  __strong typeof(_eventSender) strongSender = _eventSender;
  [strongSender sendGlobalEvent:name withParams:params];
}

- (nullable id<LynxNetworkRequestObserver>)networkRequestObserver {
  __strong typeof(_eventSender) strongSender = _eventSender;
  id<LynxBaseInspectorController> inspectorController = nil;
  if ([strongSender isKindOfClass:LynxContext.class]) {
    inspectorController = [(LynxContext *)strongSender getLynxView].baseInspectorController;
  } else if ([strongSender isKindOfClass:LynxBackgroundRuntime.class]) {
    inspectorController = [(LynxBackgroundRuntime *)strongSender devtool].baseInspectorController;
  }
  return inspectorController.networkRequestObserver;
}
@end

@implementation LynxHttpStreamingDelegate {
  LynxFetchModuleEventSender *_eventSender;
  NSString *_streamingId;
  id<LynxNetworkRequestObserver> _networkObserver;
  NSString *_networkRequestId;
}

- (instancetype)initWithParam:(LynxFetchModuleEventSender *)sender
              withStreamingId:(NSString *)streamingId {
  return [self initWithParam:sender
             withStreamingId:streamingId
             networkObserver:nil
            networkRequestId:@""];
}

- (instancetype)initWithParam:(LynxFetchModuleEventSender *)sender
              withStreamingId:(NSString *)streamingId
              networkObserver:(nullable id<LynxNetworkRequestObserver>)networkObserver
             networkRequestId:(NSString *)networkRequestId {
  if (self = [super init]) {
    _eventSender = sender;
    _streamingId = streamingId;
    _networkObserver = networkObserver;
    _networkRequestId = networkRequestId;
  }
  return self;
}

- (void)onData:(NSData *)bytes {
  if (_networkRequestId.length > 0) {
    [_networkObserver dataReceived:_networkRequestId data:bytes];
  }
  [_eventSender sendGlobalEvent:_streamingId
                     withParams:@[ @{
                       @"event" : @"onData",
                       @"data" : bytes,
                     } ]];
}
- (void)onEnd {
  if (_networkRequestId.length > 0) {
    [_networkObserver loadingFinished:_networkRequestId];
  }
  [_eventSender sendGlobalEvent:_streamingId withParams:@[ @{@"event" : @"onEnd"} ]];
}
- (void)onError:(NSString *)error {
  if (_networkRequestId.length > 0) {
    [_networkObserver loadingFailed:_networkRequestId errorText:error canceled:NO];
  }
  [_eventSender sendGlobalEvent:_streamingId
                     withParams:@[ @{
                       @"event" : @"onError",
                       @"error" : error,
                     } ]];
}

static NSString *const ERROR_STREAMING_MALFORMED_RESPONSE = @"errorStreamingMalformedResponse";

// find chunck length follow by '\r\n'
- (void)getStreamingLength:(NSMutableData *)buffer
                 chunkSize:(NSInteger *)chunkSize
                   nextIdx:(NSUInteger *)nextIdx {
  const uint8_t *bytes = buffer.bytes;
  NSUInteger length = buffer.length;
  NSUInteger i = 0;
  for (; i < length - 1; ++i) {
    if (bytes[i] == '\r') {
      break;
    }
  }
  if (i == length - 1) {
    return;
  }
  if (bytes[i + 1] != '\n') {
    [self onError:ERROR_STREAMING_MALFORMED_RESPONSE];
    return;
  }

  char temp[i + 1];
  memcpy(temp, bytes, i);
  temp[i] = '\0';
  *chunkSize = strtoul(temp, NULL, 16);
  *nextIdx = (i + 2);
}

// send chunck content follow by '\r\n'
- (void)streamingChunk:(NSMutableData *)buffer
             chunkSize:(NSInteger)chunkSize
               nextIdx:(NSUInteger)nextIdx {
  const uint8_t *bytes = buffer.bytes;
  NSData *chunkData = [buffer subdataWithRange:NSMakeRange(nextIdx, chunkSize)];
  nextIdx = nextIdx + chunkSize;
  if (bytes[nextIdx] != '\r' || bytes[nextIdx + 1] != '\n') {
    [self onError:ERROR_STREAMING_MALFORMED_RESPONSE];
    return;
  }

  [self onData:chunkData];

  NSUInteger totalRemove = nextIdx + 2;
  NSData *newData = [buffer subdataWithRange:NSMakeRange(totalRemove, buffer.length - totalRemove)];
  [buffer setData:newData];
}

- (BOOL)sendSseChunkIfComplete:(NSMutableData *)buffer {
  uint8_t prev = '\0';
  uint8_t curr;
  bool sawCR = false;
  size_t byteLength = buffer.length;
  const uint8_t *bufferBytes = [buffer bytes];
  for (size_t i = 0; i < byteLength; ++i) {
    curr = bufferBytes[i];
    if (curr == '\r') {
      if (!sawCR) {
        sawCR = true;
      } else {
        prev = 0;
      }
      continue;
    }
    sawCR = false;
    if (prev == '\n' && curr == '\n') {
      [self onData:[buffer subdataWithRange:NSMakeRange(0, i + 1)]];
      [buffer setData:[buffer subdataWithRange:NSMakeRange(i + 1, byteLength - i - 1)]];
      return YES;
    }
    prev = curr;
  }

  return NO;
}

// streaming chunk split by '\n\n'
// see: https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events
- (void)processSseData:(NSMutableData *)buffer withData:(NSData *)data {
  [buffer appendData:data];

  while ([self sendSseChunkIfComplete:buffer]) {
  }
}

// split chunck defined by `Transfer-Encoding: chunked`:
// see: https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Transfer-Encoding
- (void)processChunkedData:(NSMutableData *)buffer withData:(NSData *)data {
  [buffer appendData:data];

  while (true) {
    NSInteger chunkSize = -1;
    NSUInteger nextIdx = 0;

    [self getStreamingLength:buffer chunkSize:&chunkSize nextIdx:&nextIdx];
    // not enough data for both chunk-length and chunk-content, return to wait more data
    if (chunkSize == -1 || buffer.length < nextIdx + chunkSize + 2) {
      return;
    }

    if (chunkSize == 0) {
      [self onEnd];
      return;
    }

    [self streamingChunk:buffer chunkSize:chunkSize nextIdx:nextIdx];
  }
}

- (void)processStreamingData:(NSData *)data {
  [self onData:data];
}

@end
