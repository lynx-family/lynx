// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { CommonPerformance } from "../common/performance";
import { PerformanceEntry } from "./lynx-performance-entry";

export interface SetupTimingInfo {
  create_lynx_start: number;
  create_lynx_end: number;
  load_core_start: number;
  load_core_end: number;
  load_app_start: number;
  load_app_end: number;
  load_template_start: number;
  load_template_end: number;
  decode_start: number;
  decode_end: number;
  lepus_excute_start: number;
  lepus_excute_end: number;
  set_init_data_start: number;
  set_init_data_end: number;
  data_processor_start: number;
  data_processor_end: number;
  create_vdom_start: number;
  create_vdom_end: number;
  dispatch_start: number;
  dispatch_end: number;
  layout_start: number;
  layout_end: number;
  ui_operation_flush_start: number;
  ui_operation_flush_end: number;
  draw_end: number;
  [key: string]: unknown;
}
export interface UpdateTimingInfo {
  set_state_trigger?: number;
  create_vdom_start?: number;
  create_vdom_end?: number;
  dispatch_start?: number;
  dispatch_end?: number;
  layout_start?: number;
  layout_end?: number;
  ui_operation_flush_start?: number;
  ui_operation_flush_end?: number;
  draw_end: number;
  [key: string]: unknown;
}
export interface ExtraTimingInfo {
  prepare_template_start?: number;
  prepare_template_end?: number;
  container_init_start?: number;
  container_init_end?: number;
  open_time?: number;
  [key: string]: unknown;
}

export interface MetricsTimingInfo {
  tti?: number;
  lynx_tti?: number;
  total_tti?: number;
  fcp?: number;
  lynx_fcp?: number;
  total_fcp?: number;
  actual_fmp?: number;
  lynx_actual_fmp?: number;
  total_actual_fmp?: number;
  [key: string]: unknown;
}
export interface TimingInfo {
  extra_timing: ExtraTimingInfo;
  setup_timing: SetupTimingInfo;
  update_timings: {
    [key: string]: UpdateTimingInfo;
  };
  metrics: MetricsTimingInfo;
  has_reload: boolean;
  thread_strategy: number;
  url: string;
  [key: string]: unknown;
}
export interface TimingListener {
  onSetup: (info: TimingInfo) => void;
  onUpdate: (info: TimingInfo) => void;
}

export enum MemoryUsageQueryStatus {
  /**
   * Monitoring is enabled, the instance was found, and a best-effort memory
   * collection was performed. This does not guarantee a complete or fresh
   * snapshot.
   */
  OK = 0,
  /** Memory monitoring is disabled. */
  MONITORING_DISABLED = 1,
  /** The instance does not exist or has already been destroyed. */
  INVALID_INSTANCE = 2,
}

/**
 * A best-effort snapshot of the current page's memory usage.
 *
 * Always require `status === MemoryUsageQueryStatus.OK` before reading the
 * other properties. Even with `OK`, unsupported, unavailable or failed sources 
 * are returned as zero. A numeric field is consumable only when its value is 
 * greater than zero.
 */
export interface MemoryUsage {
  /** The query result. Check this before using any other property. */
  status: MemoryUsageQueryStatus;
  /**
   * The sum of the available page-attributed components: `elementBytes`,
   * `mtsBytes`, `btsBytes`, and `uiBytes`.
   *
   * This value can be incomplete even when `status` is `OK`:
   *
   * - UI memory is included only when UI memory sampling is enabled and the
   *   current process is selected.
   * - MTS memory is included only for a QuickJS-backed MTS runtime.
   * - For a non-shared BTS VM, BTS memory is included when the VM heap size is
   *   available.
   * - For a shared BTS VM, BTS memory is included only for QuickJS with a valid
   *   per-page memory slot. Slots 0-2 are reserved, so one VM can attribute at
   *   most 253 page allocations over its lifetime. Additional pages use the
   *   shared overflow slot and cannot report page-specific BTS memory.
   *
   * `btsHeapBytes` is not added separately. For a shared VM it describes the
   * whole VM rather than this page. Consume this field only when it is greater
   * than zero.
   */
  totalBytes: number;
  /** Page element memory in bytes. Consume only when greater than zero. */
  elementBytes: number;
  /** Number of page elements. Consume only when greater than zero. */
  elementCount: number;
  /**
   * Page MTS heap memory in bytes. It is available only for a QuickJS-backed
   * MTS runtime. Consume only when greater than zero.
   */
  mtsBytes: number;
  /**
   * Page-attributed BTS memory in bytes. For a non-shared BTS VM this is its
   * available whole heap. For a shared VM this is available only from a valid
   * QuickJS per-page memory slot. Consume only when greater than zero.
   */
  btsBytes: number;
  /**
   * The whole BTS VM heap in bytes. For a shared VM this includes other pages
   * and VM-wide memory and is not the current page's BTS usage. It can be
   * available while `btsBytes` and its contribution to `totalBytes` are not. It
   * is zero when the runtime cannot provide a heap size, including JSC, or
   * collection fails. Consume only when greater than zero.
   */
  btsHeapBytes: number;
  /**
   * Page UI memory in bytes. It is available only when UI memory sampling is
   * enabled and the current process is selected. Consume only when greater than
   * zero.
   */
  uiBytes: number;
  /**
   * Whether this page uses a shared BTS VM. Valid only when `status` is `OK`;
   * `true` does not imply that `btsBytes` is available.
   */
  btsShared: boolean;
}

export interface Performance extends CommonPerformance {
  addTimingListener(listener: TimingListener): void;
  removeTimingListener(listener: TimingListener): void;
  removeAllTimingListener(): void;
  createObserver(callback: PerformanceCallback): PerformanceObserver;
  /**
   * Asynchronously reads a best-effort current page memory snapshot. Always
   * check `MemoryUsage.status` before using the returned memory fields.
   */
  getMemoryUsage(callback: (usage: MemoryUsage) => void): void;
}

export type PerformanceCallback = (entry: PerformanceEntry) => void;
export interface PerformanceObserver {
  observe(name: string[]): void;
  disconnect(): void;
  onPerformanceEvent(entry: PerformanceEntry): void
}
