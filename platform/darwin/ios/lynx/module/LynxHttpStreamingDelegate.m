// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxHttpStreamingDelegate.h>
#import "LynxFetchModule.h"

@implementation LynxFetchModuleEventSender
- (void)sendGlobalEvent:(nonnull NSString *)name withParams:(nullable NSArray *)params {
  __strong typeof(_eventSender) strongSender = _eventSender;
  [strongSender sendGlobalEvent:name withParams:params];
}
@end

@implementation LynxHttpStreamingDelegate {
  LynxFetchModuleEventSender *_eventSender;
  NSString *_streamingId;
}

- (instancetype)initWithParam:(LynxFetchModuleEventSender *)sender
              withStreamingId:(NSString *)streamingId {
  if (self = [super init]) {
    _eventSender = sender;
    _streamingId = streamingId;
  }
  return self;
}

- (void)onData:(NSData *)bytes {
  [_eventSender sendGlobalEvent:_streamingId
                     withParams:@[ @{
                       @"event" : @"onData",
                       @"data" : bytes,
                     } ]];
}
- (void)onEnd {
  [_eventSender sendGlobalEvent:_streamingId withParams:@[ @{@"event" : @"onEnd"} ]];
}
- (void)onError:(NSString *)error {
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

  char temp[i];
  memcpy(temp, bytes, i);
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

@end
