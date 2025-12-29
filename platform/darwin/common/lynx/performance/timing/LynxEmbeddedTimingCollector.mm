// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEmbeddedTimingCollector.h"
#import <Lynx/LynxPerformanceEntryConverter.h>
#include "core/services/timing_handler/timing_constants.h"

@interface LynxEmbeddedTimingCollector ()

@property(nonatomic, assign) uint64_t loadBundleStartUs;
@property(nonatomic, assign) uint64_t paintEndUs;
@property(nonatomic, strong) NSMutableArray<NSNumber*>* updateDataStartUsList;
@property(nonatomic, assign) BOOL hasEmitLoadBundleEvent;

@end

@implementation LynxEmbeddedTimingCollector

- (instancetype _Nonnull)initWithObserver:(id<LynxPerformanceObserverProtocol> _Nonnull)observer {
  if (self = [super init]) {
    _observer = observer;
    _updateDataStartUsList = [NSMutableArray array];
    _hasEmitLoadBundleEvent = NO;
  }
  return self;
}

- (void)setTiming:(uint64_t)timestamp key:(NSString*)key {
  if ([key isEqualToString:@(lynx::tasm::timing::kLoadBundleStart)]) {
    self.loadBundleStartUs = timestamp;
  } else if ([key isEqualToString:@(lynx::tasm::timing::kUpdateTriggeredByNative)]) {
    [self.updateDataStartUsList addObject:@(timestamp)];
  } else if ([key isEqualToString:@(lynx::tasm::timing::kPaintEnd)]) {
    self.paintEndUs = timestamp;
    [self onPaintEnd];
  }
}

- (void)onPaintEnd {
  [self emitLoadBundleIfReady];
  [self emitUpdateDataIfReady];
}

- (void)emitLoadBundleIfReady {
  if (self.hasEmitLoadBundleEvent) {
    return;
  }
  if (self.loadBundleStartUs == 0 || self.paintEndUs == 0) {
    return;
  }
  self.hasEmitLoadBundleEvent = YES;
  NSDictionary* entryDict = @{
    @(lynx::tasm::timing::kEntryType) : @(lynx::tasm::timing::kEntryTypePipeline),
    @(lynx::tasm::timing::kEntryName) : @(lynx::tasm::timing::kLoadBundle),
    @(lynx::tasm::timing::kLoadBundleStart) : @((double)self.loadBundleStartUs / 1000.0),
    @(lynx::tasm::timing::kPipelineStart) : @((double)self.loadBundleStartUs / 1000.0),
    @(lynx::tasm::timing::kPaintEnd) : @((double)self.paintEndUs / 1000.0)
  };
  LynxPerformanceEntry* entry = [LynxPerformanceEntryConverter makePerformanceEntry:entryDict];
  [_observer onPerformanceEvent:entry];
}

- (void)emitUpdateDataIfReady {
  if (self.paintEndUs == 0) {
    return;
  }
  NSArray<NSNumber*>* starts = [self.updateDataStartUsList copy];
  [self.updateDataStartUsList removeAllObjects];
  for (NSNumber* num in starts) {
    NSDictionary* entryDict = @{
      @(lynx::tasm::timing::kEntryType) : @(lynx::tasm::timing::kEntryTypePipeline),
      @(lynx::tasm::timing::kEntryName) : @(lynx::tasm::timing::kUpdateTriggeredByNative),
      @(lynx::tasm::timing::kPipelineStart) : @((double)num.unsignedLongLongValue / 1000.0),
      @(lynx::tasm::timing::kPaintEnd) : @((double)self.paintEndUs / 1000.0)
    };
    LynxPerformanceEntry* entry = [LynxPerformanceEntryConverter makePerformanceEntry:entryDict];
    [_observer onPerformanceEvent:entry];
  }
}

@end
