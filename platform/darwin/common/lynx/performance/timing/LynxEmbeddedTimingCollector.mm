// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEmbeddedTimingCollector.h"
#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxPerformanceEntryConverter.h>
#import <Lynx/TemplateRenderCallbackProtocol.h>
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

static NSString* const kEmbeddedPerformanceEntryPipelineEvent =
    @"lynxsdk_performance_entry_pipeline";
static NSString* const kEmbeddedLynxFCP = @"lynxFcp";

@interface LynxEmbeddedTimingCollector ()

@property(nonatomic, assign) uint64_t loadBundleStartUs;
@property(nonatomic, assign) uint64_t loadBundleEndUs;
@property(nonatomic, assign) uint64_t paintEndUs;
@property(nonatomic, strong) NSMutableArray<NSNumber*>* updateDataStartUsList;
@property(nonatomic, assign) BOOL hasEmitLoadBundleEvent;
@property(nonatomic, assign) BOOL hasEmitSetupTiming;
@property(nonatomic, assign) BOOL hasReportedLoadBundleEvent;
@property(nonatomic, assign) int32_t instanceId;
@property(nonatomic, weak, nullable) id<TemplateRenderCallbackProtocol> embeddedTimingClient;

@end

@implementation LynxEmbeddedTimingCollector

- (instancetype _Nonnull)initWithObserver:(id<LynxPerformanceObserverProtocol> _Nonnull)observer {
  if (self = [super init]) {
    _observer = observer;
    _updateDataStartUsList = [NSMutableArray array];
    _hasEmitLoadBundleEvent = NO;
    _hasReportedLoadBundleEvent = NO;
    _instanceId = kUnknownInstanceId;
  }
  return self;
}

- (void)setInstanceId:(int32_t)instanceId {
  _instanceId = instanceId;
}

- (void)setTiming:(uint64_t)timestamp key:(NSString*)key {
  if ([key isEqualToString:@(lynx::tasm::timing::kLoadBundleStart)]) {
    self.loadBundleStartUs = timestamp;
    [self emitSetupTimingIfReady];
  } else if ([key isEqualToString:@(lynx::tasm::timing::kLoadBundleEnd)]) {
    self.loadBundleEndUs = timestamp;
    [self emitSetupTimingIfReady];
    [self emitLoadBundleObserverIfReady];
    [self reportLoadBundleIfReady];
  } else if ([key isEqualToString:@(lynx::tasm::timing::kUpdateTriggeredByNative)]) {
    [self.updateDataStartUsList addObject:@(timestamp)];
  } else if ([key isEqualToString:@(lynx::tasm::timing::kPaintEnd)]) {
    self.paintEndUs = timestamp;
    [self onPaintEnd];
  }
}

- (void)onPaintEnd {
  [self emitSetupTimingIfReady];
  [self emitLoadBundleObserverIfReady];
  [self emitUpdateDataIfReady];
  [self reportLoadBundleIfReady];
}

- (void)emitSetupTimingIfReady {
  id<TemplateRenderCallbackProtocol> embeddedTimingClient = self.embeddedTimingClient;
  if (self.hasEmitSetupTiming || embeddedTimingClient == nil) {
    return;
  }
  if (self.loadBundleStartUs == 0 || self.loadBundleEndUs < self.loadBundleStartUs ||
      self.paintEndUs < self.loadBundleEndUs) {
    return;
  }
  self.hasEmitSetupTiming = YES;
  double loadBundleStartMs = (double)self.loadBundleStartUs / 1000.0;
  double loadBundleEndMs = (double)self.loadBundleEndUs / 1000.0;
  double paintEndMs = (double)self.paintEndUs / 1000.0;
  NSDictionary* timingInfo = @{
    @(lynx::tasm::timing::kSetupTiming) : @{
      @(lynx::tasm::timing::kLoadBundleStartPolyfill) : @(loadBundleStartMs),
      @(lynx::tasm::timing::kLoadBundleEndPolyfill) : @(loadBundleEndMs),
      @(lynx::tasm::timing::kPaintEndPolyfill) : @(paintEndMs),
    },
    @(lynx::tasm::timing::kExtraTiming) : @{},
    @(lynx::tasm::timing::kUpdateTimings) : @{},
    @(lynx::tasm::timing::kMetrics) : @{
      @(lynx::tasm::timing::kLynxFCPPolyfill) : @(paintEndMs - loadBundleStartMs),
    },
    @(lynx::tasm::timing::kHasReload) : @NO,
  };
  [embeddedTimingClient onTimingSetup:timingInfo];
}

- (void)emitLoadBundleObserverIfReady {
  if (self.hasEmitLoadBundleEvent) {
    return;
  }
  if (self.loadBundleStartUs == 0 || self.paintEndUs < self.loadBundleStartUs) {
    return;
  }
  self.hasEmitLoadBundleEvent = YES;
  double loadBundleStartMs = (double)self.loadBundleStartUs / 1000.0;
  double paintEndMs = (double)self.paintEndUs / 1000.0;
  NSDictionary* entryDict = @{
    @(lynx::tasm::timing::kEntryType) : @(lynx::tasm::timing::kEntryTypePipeline),
    @(lynx::tasm::timing::kEntryName) : @(lynx::tasm::timing::kLoadBundle),
    @(lynx::tasm::timing::kLoadBundleStart) : @(loadBundleStartMs),
    @(lynx::tasm::timing::kPipelineStart) : @(loadBundleStartMs),
    @(lynx::tasm::timing::kPaintEnd) : @(paintEndMs)
  };
  LynxPerformanceEntry* entry = [LynxPerformanceEntryConverter makePerformanceEntry:entryDict];
  [_observer onPerformanceEvent:entry];
}

- (void)reportLoadBundleIfReady {
  if (self.hasReportedLoadBundleEvent || self.loadBundleStartUs == 0 ||
      self.loadBundleEndUs < self.loadBundleStartUs || self.paintEndUs < self.loadBundleEndUs) {
    return;
  }
  self.hasReportedLoadBundleEvent = YES;
  uint64_t loadBundleStartUs = self.loadBundleStartUs;
  uint64_t loadBundleEndUs = self.loadBundleEndUs;
  uint64_t paintEndUs = self.paintEndUs;
  int32_t instanceId = self.instanceId;
  [LynxEventReporter onEvent:kEmbeddedPerformanceEntryPipelineEvent
                  instanceId:instanceId
                propsBuilder:^NSDictionary<NSString*, NSObject*>* {
                  return @{
                    @(lynx::tasm::timing::kEntryType) : @(lynx::tasm::timing::kEntryTypePipeline),
                    @(lynx::tasm::timing::kEntryName) : @(lynx::tasm::timing::kLoadBundle),
                    kEmbeddedLynxFCP : @((double)(paintEndUs - loadBundleStartUs) / 1000.0),
                    @(lynx::tasm::timing::kLoadBundle) :
                        @((double)(loadBundleEndUs - loadBundleStartUs) / 1000.0)
                  };
                }];
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
