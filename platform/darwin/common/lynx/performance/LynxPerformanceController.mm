// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPerformanceEntryConverter.h>
#import <Lynx/LynxService.h>
#import "LynxMemoryMonitorProtocol.h"
#import "LynxPerformanceController+Native.h"
#include "base/trace/native/trace_event.h"
#include "core/services/timing_handler/timing_handler.h"
#include "core/services/trace/service_trace_event_def.h"

#include <memory>
#include <unordered_map>

using namespace lynx::shell;
using namespace lynx::tasm;

std::unique_ptr<std::unordered_map<std::string, std::string>> ConvertNSDictToUnorderedMap(
    NSDictionary<NSString*, NSString*>* _Nullable nsDict) {
  if (!nsDict) {
    return std::make_unique<std::unordered_map<std::string, std::string>>();
  }
  auto cppMap = std::make_unique<std::unordered_map<std::string, std::string>>();
  for (NSString* key in nsDict) {
    NSString* value = nsDict[key];
    cppMap->emplace([key UTF8String],  // NSString* → std::string
                    [value UTF8String]);
  }
  return cppMap;
}

@implementation LynxPerformanceController

- (instancetype _Nonnull)initWithObserver:(id<LynxPerformanceObserverProtocol> _Nonnull)observer {
  if (self = [super init]) {
    _observer = observer;
  }
  return self;
}

- (void)setNativeActor:(const std::shared_ptr<PerformanceControllerActor>&)nativeActor {
  _nativeWeakActorPtr = nativeActor;
}

#pragma mark - LynxMemoryMonitorProtocol

- (void)allocateMemory:(LynxMemoryRecordBuilder)recordBuilder {
  if (!recordBuilder) {
    return;
  }
  [self ActAsync:^(const std::unique_ptr<performance::PerformanceController>& controller) {
    LynxMemoryRecord* record = recordBuilder();
    if (record.detail) {
      controller->GetMemoryMonitor().AllocateMemory(performance::MemoryRecord(
          [record.category UTF8String], record.sizeKb, ConvertNSDictToUnorderedMap(record.detail)));
    } else {
      controller->GetMemoryMonitor().AllocateMemory(
          performance::MemoryRecord([record.category UTF8String], record.sizeKb));
    }
  }];
}

- (void)deallocateMemory:(LynxMemoryRecordBuilder)recordBuilder {
  if (!recordBuilder) {
    return;
  }
  [self ActAsync:^(const std::unique_ptr<performance::PerformanceController>& controller) {
    LynxMemoryRecord* record = recordBuilder();
    if (record.detail) {
      controller->GetMemoryMonitor().DeallocateMemory(performance::MemoryRecord(
          [record.category UTF8String], record.sizeKb, ConvertNSDictToUnorderedMap(record.detail)));
    } else {
      controller->GetMemoryMonitor().DeallocateMemory(
          performance::MemoryRecord([record.category UTF8String], record.sizeKb));
    }
  }];
}

- (void)updateMemoryUsage:(LynxMemoryRecordBuilder)recordBuilder {
  if (!recordBuilder) {
    return;
  }
  [self ActAsync:^(const std::unique_ptr<performance::PerformanceController>& controller) {
    LynxMemoryRecord* record = recordBuilder();
    if (record.detail) {
      controller->GetMemoryMonitor().UpdateMemoryUsage(performance::MemoryRecord(
          [record.category UTF8String], record.sizeKb, ConvertNSDictToUnorderedMap(record.detail)));
    } else {
      controller->GetMemoryMonitor().UpdateMemoryUsage(
          performance::MemoryRecord([record.category UTF8String], record.sizeKb));
    }
  }];
}

- (void)ActAsync:(void (^)(const std::unique_ptr<performance::PerformanceController>&))callback {
  auto actorPtr = _nativeWeakActorPtr.lock();
  if (!actorPtr) {
    return;
  }
  actorPtr->ActAsync([callback](auto& performance) { callback(performance); });
}

#pragma mark - LynxTimingCollectorProtocol
- (void)markTiming:(NSString*)key pipelineID:(nullable NSString*)pipelineID {
  [self setTiming:lynx::base::CurrentSystemTimeMicroseconds() key:key pipelineID:pipelineID];
}

- (void)setTiming:(uint64_t)timestamp key:(NSString*)key pipelineID:(nullable NSString*)pipelineID {
  if (!key) {
    return;
  }
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, TIMING_MARK + std::string([key UTF8String]),
      [pipeline_id = pipelineID ? std::string([pipelineID UTF8String]) : std::string(),
       timing_key = [key UTF8String], timestamp,
       actorPtr = _nativeWeakActorPtr.lock()](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("timing_key", timing_key);
        ctx.event()->add_debug_annotations("pipeline_id", pipeline_id);
        ctx.event()->add_debug_annotations("timestamp", std::to_string(timestamp));
        if (actorPtr) {
          ctx.event()->add_debug_annotations("instance_id",
                                             std::to_string(actorPtr->GetInstanceId()));
        }
      });
  [self ActAsync:^(const std::unique_ptr<performance::PerformanceController>& controller) {
    auto timingKey = timing::TimestampKey([key UTF8String]);
    auto timingPipelineID = pipelineID ? PipelineID([pipelineID UTF8String]) : PipelineID();
    controller->GetTimingHandler().SetTiming(timingKey, static_cast<timing::TimestampUs>(timestamp),
                                             timingPipelineID);
  }];
}

- (void)MarkPaintEndTimingIfNeeded {
  [self ActAsync:^(const std::unique_ptr<performance::PerformanceController>& controller) {
    controller->GetTimingHandler().SetPaintEndTimingIfNeeded(
        lynx::base::CurrentSystemTimeMicroseconds());
  }];
}

- (void)onPipelineStart:(NSString*)pipelineId
         pipelineOrigin:(NSString*)pipelineOrigin
              timestamp:(uint64_t)timestamp {
  [self ActAsync:^(const std::unique_ptr<performance::PerformanceController>& controller) {
    auto timingPipelineOrigin = PipelineOrigin([pipelineOrigin UTF8String]);
    auto timingPipelineId = pipelineId ? PipelineID([pipelineId UTF8String]) : PipelineID();

    controller->GetTimingHandler().OnPipelineStart(timingPipelineId, timingPipelineOrigin,
                                                   static_cast<timing::TimestampUs>(timestamp));
  }];
}

- (void)resetTimingBeforeReload {
  [self ActAsync:^(const std::unique_ptr<performance::PerformanceController>& controller) {
    controller->GetTimingHandler().ResetTimingBeforeReload();
  }];
}

#pragma mark - LynxPerformanceObserverProtocol
- (void)onPerformanceEvent:(nonnull LynxPerformanceEntry*)entry {
  [_observer onPerformanceEvent:entry];
}

@end
