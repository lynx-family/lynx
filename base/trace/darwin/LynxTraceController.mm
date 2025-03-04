// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTraceController.h"

#import "base/trace/native/platform/darwin/trace_controller_darwin.h"

static LynxTraceController *lynxTraceController = nil;

@implementation LynxTraceController {
  lynx::trace::TraceController *_traceController;
  int _sessionId;
  NSMutableArray<completeBlockType> *_completeBlocks;
}

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    lynxTraceController = [[self alloc] init];
  });
  return lynxTraceController;
}

- (instancetype)init {
  _traceController = lynx::trace::GetTraceControllerInstance();
  _completeBlocks = [[NSMutableArray alloc] init];
  _sessionId = -1;
  return self;
}

- (intptr_t)getTraceController {
  return reinterpret_cast<intptr_t>(_traceController);
}

- (void)startTrace {
  [self startTracing:nullptr config:@{}];
}

- (void)stopTrace {
  [self stopTracing];
}

- (void)startTracing:(completeBlockType)completeBlock config:(NSDictionary *)config {
  static int const kDefaultBufferSize = 40960;
  if (completeBlock != nullptr) {
    [_completeBlocks addObject:completeBlock];
  }
  if (_traceController != nullptr) {
    int bufferSize = kDefaultBufferSize;
    if ([config valueForKey:@"buffer_size"] != nil) {
      bufferSize = [config[@"buffer_size"] intValue];
    }
    auto trace_config = std::make_shared<lynx::trace::TraceConfig>();
    trace_config->buffer_size = bufferSize;
    if ([config valueForKey:@"trace_file"] != nil) {
      trace_config->file_path = [config[@"trace_file"] UTF8String];
    }
    trace_config->included_categories = {"*"};
    trace_config->excluded_categories = {"*"};
    _sessionId = _traceController->StartTracing(trace_config);
    _traceController->AddCompleteCallback(_sessionId, [self, trace_config]() {
      [self onTracingComplete:[NSString stringWithUTF8String:trace_config->file_path.c_str()]];
    });
  }
}

- (void)startTracing:(completeBlockType)completeBlock jsonConfig:(NSString *)config {
  NSData *configData = [config dataUsingEncoding:NSUTF8StringEncoding];
  NSError *error;
  NSDictionary *dict = [NSJSONSerialization JSONObjectWithData:configData
                                                       options:kNilOptions
                                                         error:&error];
  if (!error) {
    [self startTracing:completeBlock config:dict];
  } else {
    NSLog(@"Error parsing config JSON: %@", error);
  }
}

- (void)stopTracing {
  if (_traceController != nullptr) {
    _traceController->StopTracing(_sessionId);
  }
}

- (void)onTracingComplete:(NSString *)traceFile {
  for (completeBlockType block in _completeBlocks) {
    block(traceFile);
  }
}

- (void)startStartupTracingIfNeeded {
  if (_traceController != nullptr) {
    _traceController->StartStartupTracingIfNeeded();
  }
}

@end
