// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';

export enum LynxMemoryCollectionStatus {
  COMPLETED = 'completed',
  TIMEOUT = 'timeout',
  ERROR = 'error'
}

export interface LynxInstanceMemoryUsage {
  instanceId: number;
  // Native memory-monitor page key.
  pageId: string;
  url: string;
  // Page metrics are absent for destroyed instances.
  totalBytes?: number;
  elementBytes?: number;
  elementNodeCount?: number;
  viewBytes?: number;
  viewDetail?: Record<string, Object>;
  mainThreadRuntimeBytes?: number;
  backgroundThreadRuntimeBytes?: number;
  btsRuntimeGroupId: string;
  // Background runtime engine: quickjs, v8, jsvm, jsc, or unknown.
  btsRuntimeType: string;
  // Whole background runtime heap, separate from page-attributed bytes.
  btsRuntimeTotalBytes: number;
  // Latest cached slot bytes, only present for destroyed instances.
  backgroundThreadRuntimeResidualBytes?: number;
}

export interface LynxGlobalMemoryUsageResult {
  collectionStartMs: number;
  collectionStatus: LynxMemoryCollectionStatus;
  errorMessage: string;
  collectionDurationMs: number;
  // Retained metadata; the native snapshot does not wait for this timeout.
  collectionTimeoutMs: number;
  expectedInstanceCount: number;
  completedInstanceCount: number;
  totalBytes: number;
  appBytes: number;
  // Global Lynx-attributed total divided by appBytes, or zero when appBytes is unavailable.
  ratioToApp: number;
  elementBytes: number;
  elementNodeCount: number;
  viewBytes: number;
  mainThreadRuntimeBytes: number;
  backgroundThreadRuntimeBytes: number;
  instances: LynxInstanceMemoryUsage[];
  // Up to five destroyed pages per shared QuickJS VM.
  destroyedInstances: LynxInstanceMemoryUsage[];
}

export type LynxGlobalMemoryUsageCallback = (result: LynxGlobalMemoryUsageResult) => void;

/** Queries current monitored memory; disabled monitoring returns an ERROR result. */
export class LynxMemoryUsageQuery {
  private static readonly instance: LynxMemoryUsageQuery = new LynxMemoryUsageQuery();

  private constructor() {}

  public static inst(): LynxMemoryUsageQuery {
    return LynxMemoryUsageQuery.instance;
  }

  /** The callback runs on the calling ArkTS environment after the native snapshot. */
  public queryLynxGlobalMemoryUsageAsync(callback: LynxGlobalMemoryUsageCallback,
    timeoutMs: number = 2000): void {
    lynx.queryGlobalMemoryUsage((result: Object): void => {
      callback(result as LynxGlobalMemoryUsageResult);
    }, timeoutMs > 0 ? timeoutMs : 2000);
  }
}
