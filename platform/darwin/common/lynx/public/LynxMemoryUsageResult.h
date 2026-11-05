// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxMemoryCollectionStatus) {
  // The native global snapshot completed.
  LynxMemoryCollectionStatusCompleted = 0,
  // The fixed timeout fired first; the result contains only completed instances.
  LynxMemoryCollectionStatusTimeout = 1,
  // Monitoring is disabled or the query failed.
  LynxMemoryCollectionStatusError = 2,
};

@class LynxMemoryRecord;

/**
 * Memory attribution for one completed Lynx instance query.
 *
 * The object keeps page-attributed samples. The global result replaces shared
 * page BTS slots with each owning VM's whole heap exactly once.
 */
@interface LynxInstanceMemoryUsage : NSObject

// LynxShell instance id. -1 means the instance was not fully attached.
@property(nonatomic, assign) int32_t instanceId;
// Native memory-monitor page key.
@property(nonatomic, copy) NSString *pageId;
// Current template URL captured when the instance fetcher completes.
@property(nonatomic, copy) NSString *url;
// Instance-attributed total: element + view + main runtime + background runtime.
@property(nonatomic, assign) int64_t totalBytes;
// Element tree memory reported from the native element manager.
@property(nonatomic, assign) int64_t elementBytes;
// Number of element nodes used as a context signal for elementBytes.
@property(nonatomic, assign) int32_t elementNodeCount;
// iOS UI memory reported by LynxUIOwner.
@property(nonatomic, assign) int64_t viewBytes;
// Per-view-class or per-view-key memory records reported by LynxUIOwner.
@property(nonatomic, copy) NSDictionary<NSString *, LynxMemoryRecord *> *viewDetail;
// Main-thread runtime heap snapshot bytes. The query reads current heap stats without GC.
@property(nonatomic, assign) int64_t mainThreadRuntimeBytes;
// Background-thread bytes attributed to this page.
@property(nonatomic, assign) int64_t backgroundThreadRuntimeBytes;
// Current tracked slot bytes for a destroyed instance; zero for live instances.
@property(nonatomic, assign) int64_t backgroundThreadRuntimeResidualBytes;
// Background runtime group id for diagnostics. It is not a unique VM identity.
@property(nonatomic, copy) NSString *btsRuntimeGroupId;
// Background runtime engine: quickjs, v8, jsvm, jsc, or unknown.
@property(nonatomic, copy) NSString *btsRuntimeType;
// Whole background runtime heap, separate from the page-attributed bytes.
@property(nonatomic, assign) int64_t btsRuntimeTotalBytes;
@end

/**
 * Typed result returned by LynxMemoryUsageQuery's global memory query API.
 *
 * The result is one native global snapshot. Both instance count fields equal
 * the active instance count, and instances is sorted by totalBytes descending.
 */
@interface LynxGlobalMemoryUsageResult : NSObject

// Wall-clock collection start time in milliseconds.
@property(nonatomic, assign) int64_t collectionStartMs;
// The direct native global snapshot completes as one operation.
@property(nonatomic, assign) LynxMemoryCollectionStatus collectionStatus;
// Empty on success; explains why the query failed otherwise.
@property(nonatomic, copy) NSString *errorMessage;
// Elapsed collection time in milliseconds.
@property(nonatomic, assign) int64_t collectionDurationMs;
// Timeout metadata retained for API compatibility.
@property(nonatomic, assign) int64_t collectionTimeoutMs;
// Number of active native instances in the snapshot.
@property(nonatomic, assign) NSInteger expectedInstanceCount;
// Number of instance results included in this result.
@property(nonatomic, assign) NSInteger completedInstanceCount;
// Global Lynx-attributed total bytes. This excludes appBytes.
@property(nonatomic, assign) int64_t totalBytes;
// Current app physical footprint sampled when the global result is built.
@property(nonatomic, assign) int64_t appBytes;
// Global Lynx-attributed total divided by appBytes, or zero when appBytes is unavailable.
@property(nonatomic, assign) double ratioToApp;
// Aggregated element bytes across completed instances.
@property(nonatomic, assign) int64_t elementBytes;
// Aggregated element node count across completed instances.
@property(nonatomic, assign) int32_t elementNodeCount;
// Aggregated UI/view bytes across completed instances.
@property(nonatomic, assign) int64_t viewBytes;
// Aggregated main-thread runtime bytes across completed instances.
@property(nonatomic, assign) int64_t mainThreadRuntimeBytes;
// All active BTS VM heaps, counted once by native VM identity.
@property(nonatomic, assign) int64_t backgroundThreadRuntimeBytes;
// Completed instance list, sorted by instance totalBytes descending.
@property(nonatomic, copy) NSArray<LynxInstanceMemoryUsage *> *instances;
// Up to five destroyed pages per shared QuickJS VM, ordered by residual bytes within each VM.
@property(nonatomic, copy) NSArray<LynxInstanceMemoryUsage *> *destroyedInstances;

@end

NS_ASSUME_NONNULL_END
