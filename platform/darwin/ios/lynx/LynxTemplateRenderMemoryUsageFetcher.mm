// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxMemoryRecord.h>
#import <Lynx/LynxMemoryUsageResult.h>
#import <Lynx/LynxTemplateRender+Internal.h>

#import "platform/darwin/common/lynx/performance/memory/LynxMemoryUsageFetcher.h"
#import "platform/darwin/common/lynx/trace/LynxTraceEventDef.h"

#include "core/shell/performance/native_memory_usage_query.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

typedef void (^LynxTemplateRenderNativeMemoryUsageCallback)(
    const lynx::tasm::performance::NativeMemoryUsageSnapshot& snapshot);

@interface LynxTemplateRender (MemoryUsageBridge)

- (BOOL)queryNativeMemoryUsageForGlobalCollectorAsync:
    (LynxTemplateRenderNativeMemoryUsageCallback)callback;
- (NSString*)memoryUsageTemplateUrl;

@end

namespace {

int64_t LynxMemoryUsageNowMs() {
  return static_cast<int64_t>([[NSDate date] timeIntervalSince1970] * 1000);
}

}  // namespace

@interface LynxTemplateRenderInstanceMemoryUsageQuery : NSObject

+ (NSDictionary<NSString*, LynxMemoryRecord*>*)
    copyViewMemoryRecords:(NSDictionary<NSString*, LynxMemoryRecord*>*)records
               totalBytes:(int64_t*)totalBytes;

- (instancetype)initWithInstanceId:(int32_t)instanceId
                               url:(NSString*)url
                          callback:(LynxMemoryUsageFetcherCallback)callback;
- (void)completeNativeSnapshot:(const lynx::tasm::performance::NativeMemoryUsageSnapshot&)snapshot;
- (void)completeViewSnapshotWithBytes:(int64_t)viewBytes
                               detail:(NSDictionary<NSString*, LynxMemoryRecord*>*)detail;
- (void)completeInvalidSnapshot;

@end

@implementation LynxTemplateRenderInstanceMemoryUsageQuery {
  int32_t _instanceId;
  int64_t _queryStartMs;
  NSString* _url;
  LynxMemoryUsageFetcherCallback _callback;
  BOOL _nativeDone;
  BOOL _viewDone;
  BOOL _finished;
  BOOL _invalid;
  int64_t _elementBytes;
  int32_t _elementNodeCount;
  int64_t _mainThreadRuntimeBytes;
  int64_t _backgroundThreadRuntimeBytes;
  NSString* _btsRuntimeGroupId;
  int64_t _viewBytes;
  NSDictionary<NSString*, LynxMemoryRecord*>* _viewDetail;
}

+ (NSDictionary<NSString*, LynxMemoryRecord*>*)
    copyViewMemoryRecords:(NSDictionary<NSString*, LynxMemoryRecord*>*)records
               totalBytes:(int64_t*)totalBytes {
  NSMutableDictionary<NSString*, LynxMemoryRecord*>* details = [NSMutableDictionary dictionary];
  int64_t bytes = 0;
  for (NSString* key in records) {
    LynxMemoryRecord* record = records[key];
    if (!record) {
      continue;
    }
    bytes += record.sizeBytes;
    LynxMemoryRecord* recordCopy = [record copy];
    NSString* detailKey = key.length > 0 ? key : recordCopy.category;
    if (detailKey.length > 0) {
      details[detailKey] = recordCopy;
    }
  }
  if (totalBytes) {
    *totalBytes = bytes;
  }
  return [details copy];
}

- (instancetype)initWithInstanceId:(int32_t)instanceId
                               url:(NSString*)url
                          callback:(LynxMemoryUsageFetcherCallback)callback {
  self = [super init];
  if (self) {
    _instanceId = instanceId;
    _queryStartMs = LynxMemoryUsageNowMs();
    _url = [url copy] ?: @"";
    _callback = [callback copy];
    _btsRuntimeGroupId = @"";
    _viewDetail = @{};
  }
  return self;
}

- (void)completeNativeSnapshot:(const lynx::tasm::performance::NativeMemoryUsageSnapshot&)snapshot {
  if (_finished || _nativeDone) {
    return;
  }
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_MEMORY_NATIVE_RESULT, INSTANCE_ID, _instanceId,
                "element_bytes", snapshot.element_bytes_, "main_thread_runtime_bytes",
                snapshot.main_thread_runtime_bytes_, "background_thread_runtime_bytes",
                snapshot.background_thread_runtime_bytes_);
    _elementBytes = snapshot.element_bytes_;
    _elementNodeCount = snapshot.element_node_count_;
    _mainThreadRuntimeBytes = snapshot.main_thread_runtime_bytes_;
    _backgroundThreadRuntimeBytes = snapshot.background_thread_runtime_bytes_;
    NSString* runtimeGroupId =
        snapshot.bts_runtime_group_id_.empty()
            ? @""
            : [NSString stringWithUTF8String:snapshot.bts_runtime_group_id_.c_str()];
    _btsRuntimeGroupId = runtimeGroupId ?: @"";
    _nativeDone = YES;
  }
  [self finishIfReady];
}

- (void)completeViewSnapshotWithBytes:(int64_t)viewBytes
                               detail:(NSDictionary<NSString*, LynxMemoryRecord*>*)detail {
  if (_finished || _viewDone) {
    return;
  }
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_MEMORY_VIEW_RESULT, INSTANCE_ID, _instanceId,
                "view_bytes", viewBytes, "view_detail_count", detail.count);
    _viewBytes = viewBytes;
    // View records are already detached on the main thread. Keep a private map
    // snapshot while waiting for the native half of this per-instance query.
    _viewDetail = [detail copy] ?: @{};
    _viewDone = YES;
  }
  [self finishIfReady];
}

- (void)completeInvalidSnapshot {
  if (_finished || _viewDone) {
    return;
  }
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_MEMORY_INVALID, INSTANCE_ID, _instanceId);
    _invalid = YES;
    _viewDone = YES;
  }
  [self finishIfReady];
}

- (void)finishIfReady {
  if (_finished || !_nativeDone || !_viewDone) {
    return;
  }
  _finished = YES;
  if (_invalid) {
    LynxMemoryUsageFetcherCallback callback = _callback;
    _callback = nil;
    if (callback) {
      callback(nil);
    }
    return;
  }

  LynxInstanceMemoryUsage* result = [[LynxInstanceMemoryUsage alloc] init];
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_MEMORY_COMPLETE, INSTANCE_ID, _instanceId);
    result.instanceId = _instanceId;
    // TODO(lynx-memory): Replace this temporary URL-derived identity with the real page id once it
    // is exposed to the instance memory fetcher.
    result.pageId = _url;
    result.url = _url;
    result.elementBytes = _elementBytes;
    result.elementNodeCount = _elementNodeCount;
    result.viewBytes = _viewBytes;
    result.mainThreadRuntimeBytes = _mainThreadRuntimeBytes;
    result.backgroundThreadRuntimeBytes = _backgroundThreadRuntimeBytes;
    result.totalBytes =
        _elementBytes + _viewBytes + _mainThreadRuntimeBytes + _backgroundThreadRuntimeBytes;
    result.viewDetail = _viewDetail;
    result.btsRuntimeGroupId = _btsRuntimeGroupId;
  }
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_MEMORY_TOTAL, INSTANCE_ID,
                      result.instanceId, "total_bytes", result.totalBytes, "view_detail_count",
                      result.viewDetail.count, "duration_ms",
                      LynxMemoryUsageNowMs() - _queryStartMs);
  LynxMemoryUsageFetcherCallback callback = _callback;
  _callback = nil;
  if (callback) {
    callback(result);
  }
}

@end

@interface LynxTemplateRenderMemoryUsageFetcher : NSObject <LynxMemoryUsageFetcher>

- (instancetype)initWithTemplateRender:(LynxTemplateRender*)templateRender;

@end

@implementation LynxTemplateRenderMemoryUsageFetcher {
  __weak LynxTemplateRender* _templateRender;
}

- (instancetype)initWithTemplateRender:(LynxTemplateRender*)templateRender {
  self = [super init];
  if (self) {
    _templateRender = templateRender;
  }
  return self;
}

- (void)queryMemoryUsageAsync:(LynxMemoryUsageFetcherCallback)callback {
  if (!callback) {
    return;
  }

  LynxTemplateRender* templateRender = _templateRender;
  if (!templateRender) {
    callback(nil);
    return;
  }

  int32_t instanceId = [templateRender instanceId];
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_MEMORY_QUERY, INSTANCE_ID, instanceId);
  LynxTemplateRenderInstanceMemoryUsageQuery* query =
      [[LynxTemplateRenderInstanceMemoryUsageQuery alloc]
          initWithInstanceId:instanceId
                         url:[templateRender memoryUsageTemplateUrl]
                    callback:callback];

  BOOL nativeQueryStarted =
      [templateRender queryNativeMemoryUsageForGlobalCollectorAsync:^(
                          const lynx::tasm::performance::NativeMemoryUsageSnapshot& snapshot) {
        [query completeNativeSnapshot:snapshot];
      }];
  if (!nativeQueryStarted) {
    // The render is already unavailable for native collection. Treat it as an
    // invalid instance instead of mixing a zero native snapshot with a later
    // main-thread view sample from a tearing-down or reused render.
    [LynxEventReporter
        delayRunOnReportThread:^{
          [query completeInvalidSnapshot];
          lynx::tasm::performance::NativeMemoryUsageSnapshot snapshot;
          [query completeNativeSnapshot:snapshot];
        }
                       delayMs:0];
    return;
  }

  __weak LynxTemplateRender* weakTemplateRender = templateRender;
  dispatch_async(dispatch_get_main_queue(), ^{
    LynxTemplateRender* strongTemplateRender = weakTemplateRender;
    LynxUIOwner* uiOwner = [strongTemplateRender uiOwner];
    if (!strongTemplateRender || [strongTemplateRender instanceId] != instanceId || !uiOwner) {
      [LynxEventReporter
          delayRunOnReportThread:^{
            [query completeInvalidSnapshot];
          }
                         delayMs:0];
      return;
    }

    NSDictionary<NSString*, LynxMemoryRecord*>* details = nil;
    int64_t viewBytes = 0;
    {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_MEMORY_VIEW_QUERY, INSTANCE_ID, instanceId);
      NSDictionary<NSString*, LynxMemoryRecord*>* records = [[uiOwner getMemoryUsage] copy] ?: @{};
      // The UI owner builds mutable records on the main thread. Copy each record before handing the
      // snapshot to the report thread so later UI sampling cannot mutate this query's result.
      details = [LynxTemplateRenderInstanceMemoryUsageQuery copyViewMemoryRecords:records
                                                                       totalBytes:&viewBytes];
    }

    [LynxEventReporter
        delayRunOnReportThread:^{
          [query completeViewSnapshotWithBytes:viewBytes detail:details];
        }
                       delayMs:0];
  });
}

@end
