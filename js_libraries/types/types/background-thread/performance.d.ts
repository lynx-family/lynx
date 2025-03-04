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

export interface Performance extends CommonPerformance {
  addTimingListener(listener: TimingListener): void;
  removeTimingListener(listener: TimingListener): void;
  removeAllTimingListener(): void;
  createObserver(callback: PerformanceCallback): PerformanceObserver;
}


export type PerformanceCallback = (entry: PerformanceEntry) => void;
export interface PerformanceObserver {
  observe(name: string[]): void;
  disconnect(): void;
  onPerformanceEvent(entry: PerformanceEntry): void
}
