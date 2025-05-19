// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPerformanceEntryConverter.h>
#import <Lynx/LynxService.h>
#import "LynxPerformanceController+Native.h"

#include <memory>
#include <unordered_map>

std::unique_ptr<std::unordered_map<std::string, std::string>> ConvertNSDictToUnorderedMap(
    NSMutableDictionary<NSString*, NSString*>* _Nullable nsDict) {
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

- (void)setNativeActor:
    (const std::weak_ptr<lynx::shell::LynxActor<lynx::tasm::performance::PerformanceController>>&)
        nativeActor {
  _weakNativeActorPtr = nativeActor;
}

+ (void)setForceEnable:(BOOL)forceEnable {
  lynx::tasm::performance::MemoryMonitor::SetForceEnable(forceEnable);
}

#pragma mark - LynxMemoryMonitorProtocol

- (BOOL)enableMemoryMonitor {
  return lynx::tasm::performance::MemoryMonitor::Enable();
}

- (void)allocateMemoryWithCategory:(NSString* _Nonnull)category
                            sizeKb:(float)sizeKb
                            detail:(NSDictionary<NSString*, NSString*>* _Nullable)detail {
  [self ActAsync:^(lynx::tasm::performance::PerformanceController* controller) {
    if (detail) {
      controller->GetMemoryMonitor().AllocateMemory(lynx::tasm::performance::MemoryRecord(
          [category UTF8String], sizeKb, ConvertNSDictToUnorderedMap(detail)));
    } else {
      controller->GetMemoryMonitor().AllocateMemory(
          lynx::tasm::performance::MemoryRecord([category UTF8String], sizeKb));
    }
  }];
}

- (void)deallocateMemoryWithCategory:(NSString* _Nonnull)category
                              sizeKb:(float)sizeKb
                              detail:(NSDictionary<NSString*, NSString*>* _Nullable)detail {
  [self ActAsync:^(lynx::tasm::performance::PerformanceController* controller) {
    if (detail) {
      controller->GetMemoryMonitor().DeallocateMemory(lynx::tasm::performance::MemoryRecord(
          [category UTF8String], sizeKb, ConvertNSDictToUnorderedMap(detail)));
    } else {
      controller->GetMemoryMonitor().DeallocateMemory(
          lynx::tasm::performance::MemoryRecord([category UTF8String], sizeKb));
    }
  }];
}

- (void)updateMemoryUsageWithCategory:(NSString* _Nonnull)category
                               sizeKb:(float)sizeKb
                               detail:(NSDictionary<NSString*, NSString*>* _Nullable)detail {
  [self ActAsync:^(lynx::tasm::performance::PerformanceController* controller) {
    if (detail) {
      controller->GetMemoryMonitor().UpdateMemoryUsage(lynx::tasm::performance::MemoryRecord(
          [category UTF8String], sizeKb, ConvertNSDictToUnorderedMap(detail)));
    } else {
      controller->GetMemoryMonitor().UpdateMemoryUsage(
          lynx::tasm::performance::MemoryRecord([category UTF8String], sizeKb));
    }
  }];
}

- (void)ActAsync:(void (^)(lynx::tasm::performance::PerformanceController*))callback {
  if (!callback) {
    return;
  }
  auto lockedNativeActorPtr = _weakNativeActorPtr.lock();
  if (lockedNativeActorPtr) {
    lockedNativeActorPtr->ActAsync([callback](auto& performance) { callback(performance); });
  }
}

#pragma mark - LynxPerformanceObserverProtocol
- (void)onPerformanceEvent:(nonnull LynxPerformanceEntry*)entry {
  [_observer onPerformanceEvent:entry];
}

@end
