// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_DARWIN_LYNXTRACECONTROLLER_H_
#define BASE_TRACE_DARWIN_LYNXTRACECONTROLLER_H_

typedef void (^completeBlockType)(NSString* traceFile);

@interface LynxTraceController : NSObject
+ (instancetype)sharedInstance;
- (intptr_t)getTraceController;
- (void)startTracing:(completeBlockType)completeBlock config:(NSDictionary*)config;
- (void)startTracing:(completeBlockType)completeBlock jsonConfig:(NSString*)config;
- (void)stopTracing;
- (void)startTrace;
- (void)stopTrace;
- (void)onTracingComplete:(NSString*)traceFile;
- (void)startStartupTracingIfNeeded;
@end

#endif  // BASE_TRACE_DARWIN_LYNXTRACECONTROLLER_H_
