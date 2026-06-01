// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxMemoryCollectionStatus) {
  // Every expected Lynx instance fetcher returned before the fixed timeout.
  LynxMemoryCollectionStatusCompleted = 0,
  // The fixed timeout fired first; the result contains only completed instances.
  LynxMemoryCollectionStatusTimeout = 1,
};

@class LynxMemoryRecord;

/**
 * Memory attribution for one completed Lynx instance query.
 *
 * The object intentionally keeps the instance-level raw samples. The global
 * result may deduplicate shared background runtime bytes when calculating the
 * global total, but this object continues to expose what the instance sampled.
 */
@interface LynxInstanceMemoryUsage : NSObject

// LynxShell instance id. -1 means the instance was not fully attached.
@property(nonatomic, assign) int32_t instanceId;
// TODO(lynx-memory): Replace this temporary view-derived identity with the real page id once the
// page id source is available to the instance fetcher.
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
// Background-thread runtime heap snapshot bytes. The query reads current heap stats without GC.
@property(nonatomic, assign) int64_t backgroundThreadRuntimeBytes;
// Background runtime group id. The global total deduplicates non-empty shared group ids.
@property(nonatomic, copy) NSString *btsRuntimeGroupId;

@end

/**
 * Typed result returned by LynxMemoryUsageQuery's global memory query API.
 *
 * The result is a single request snapshot. expectedInstanceCount is frozen at
 * request start, completedInstanceCount counts only fetchers that returned a
 * complete instance result, and instances is sorted by totalBytes descending.
 */
@interface LynxGlobalMemoryUsageResult : NSObject

// Wall-clock collection start time in milliseconds.
@property(nonatomic, assign) int64_t collectionStartMs;
// Whether all expected fetchers completed or the fixed timeout produced a partial result.
@property(nonatomic, assign) LynxMemoryCollectionStatus collectionStatus;
// Elapsed collection time in milliseconds.
@property(nonatomic, assign) int64_t collectionDurationMs;
// Fixed timeout used by the first iOS implementation.
@property(nonatomic, assign) int64_t collectionTimeoutMs;
// Number of live fetchers snapshotted at request start.
@property(nonatomic, assign) NSInteger expectedInstanceCount;
// Number of completed instance results included in this result.
@property(nonatomic, assign) NSInteger completedInstanceCount;
// Global Lynx-attributed total bytes. This excludes appBytes and deduplicates shared BTS runtime.
@property(nonatomic, assign) int64_t totalBytes;
// Current app physical footprint sampled when the global result is built.
@property(nonatomic, assign) int64_t appBytes;
// totalBytes divided by appBytes. Zero when appBytes is unavailable.
@property(nonatomic, assign) double ratioToApp;
// Aggregated element bytes across completed instances.
@property(nonatomic, assign) int64_t elementBytes;
// Aggregated element node count across completed instances.
@property(nonatomic, assign) int32_t elementNodeCount;
// Aggregated UI/view bytes across completed instances.
@property(nonatomic, assign) int64_t viewBytes;
// Aggregated main-thread runtime bytes across completed instances.
@property(nonatomic, assign) int64_t mainThreadRuntimeBytes;
// Aggregated background runtime bytes with shared btsRuntimeGroupId values deduplicated.
@property(nonatomic, assign) int64_t backgroundThreadRuntimeBytes;
// Completed instance list, sorted by instance totalBytes descending.
@property(nonatomic, copy) NSArray<LynxInstanceMemoryUsage *> *instances;

@end

NS_ASSUME_NONNULL_END
