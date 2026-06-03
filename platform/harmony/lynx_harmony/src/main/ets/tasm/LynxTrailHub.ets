// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import { LLog } from '../base/LLog';
import { LynxServiceCenter, LynxServiceType } from './service/LynxServiceCenter';
import { ILynxTrailService } from '../tasm/service/ILynxTrailService';
import common from '@ohos.app.ability.common';

const TAG = 'LynxTrailHub';

/**
 * Notice: make sure LynxTrailKey contains all the experiment keys of LynxEnv::Key in C++
 */
export enum LynxTrailKey {
  ENABLE_QUICKJS_CACHE = 'enable_quickjs_cache',
  ANDROID_DISABLE_QUICKJS_CODE_CACHE = 'ANDROID_DISABLE_QUICKJS_CODE_CACHE',
  DISABLE_TRACING_GC = 'DISABLE_TRACING_GC',
  LAYOUT_PERFORMANCE_ENABLE = 'layout_performance_enable',
  ENABLE_V8 = 'enable_v8',
  ENABLE_PIPER_MONITOR = 'enablePiperMonitor',
  ENABLE_DOM_TREE = 'enable_dom_tree',
  ENABLE_VSYNC_ALIGNED_FLUSH = 'enable_vsync_aligned_flush',
  ENABLE_VSYNC_ALIGNED_FLUSH_LOCAL = 'enable_vsync_aligned_flush_local',
  ENABLE_GLOBAL_FEATURE_SWITCH_STATISTIC = 'ENABLE_GLOBAL_FEATURE_SWITCH_STATISTIC',
  ENABLE_JSB_TIMING = 'enable_jsb_timing',
  ENABLE_FEATURE_COUNTER = 'ENABLE_FEATURE_COUNTER',
  ENABLE_ASYNC_JSB_TIMING = 'enable_async_jsb_timing',
  ENABLE_LONG_TASK_TIMING = 'enable_long_task_timing',
  ENABLE_QUICKJS_DEBUG = 'enable_quickjs_debug',
  ENABLE_TABLE_DEEP_CHECK = 'enable_table_deep_check',
  DISABLE_LEPUSNG_OPTIMIZE = 'DISABLE_LEPUSNG_OPTIMIZE',
  FORCE_LYNX_NETWORK = 'FORCE_LYNX_NETWORK',
  FORCE_LYNX_NETWORK_ALLOWLIST = 'FORCE_LYNX_NETWORK_ALLOWLIST',
  FORCE_LYNX_NETWORK_BLOCKLIST = 'FORCE_LYNX_NETWORK_BLOCKLIST',
  TRAIL_ASYNC_HYDRATION = 'trail_async_hydration',
  DISABLE_LAZY_CSS_DECODE = 'disable_lazy_css_decode',
  DECOUPLE_LAYOUTNODE_FROM_ELEMENT = 'decouple_layoutnode_from_element',
  TRAIL_REMOVE_COMPONENT_EXTRA_DATA = 'trialRemoveComponentExtraData',
  REACT_LYNX_SETTINGS = 'lynxsdk_react_lynx_settings',
  TRAIL_LYNX_RESOURCE_SERVICE_PROVIDER = 'trialLynxResourceServiceProvider',
  ENABLE_COMPONENT_ASYNC_DECODE = 'enableComponentAsyncDecode',
  V8_HEAP_SIZE = 'V8_HEAP_SIZE',
  ENABLE_ASYNC_THREAD_CACHE = 'enable_async_thread_cache',
  MULTI_TASM_THREAD_SIZE = 'multi_tasm_thread_size',
  MULTI_LAYOUT_THREAD_SIZE = 'multi_layout_thread_size',
  ENABLE_FLUENCY_TRACE = 'ENABLE_FLUENCY_TRACE',
  ENABLE_REPORT_THREADED_ELEMENT_FLUSH_STATISTIC = 'enable_report_threaded_element_flush_statistic',
  GLOBAL_QUICK_CONTEXT_POOL_SIZE = 'global_quick_context_pool_size',
  CLIP_RADIUS_FLATTEN = 'clip_radius_flatten',
  ENABLE_UI_OP_BATCH = 'enable_ui_op_batch',
  ENABLE_LAYOUT_ONLY_STATISTIC = 'enable_layout_only_statistic',
  DISABLE_PERF_COLLECTOR = 'disable_perf_collector',
  VSYNC_TRIGGERED_FROM_UI_THREAD_ANDROID = 'lynx_vsync_triggered_from_ui_thread_android',
  VSYNC_POST_TASK_BY_EMERGENCY = 'lynx_vsync_post_task_by_emergency',
  ENABLE_USE_MAP_BUFFER_FOR_UI_PROPS = 'use_map_buffer_for_ui_props',
  ENABLE_LAZY_IMPORT_CSS = 'enable_lazy_import_css',
  DISABLE_ONCE_DYNAMIC_CSS = 'disable_once_dynamic_css',
  ENABLE_REPORT_DYNAMIC_COMPONENT_EVENT = 'enable_report_dynamic_component_event',
  BYTECODE_MAX_SIZE = 'bytecode_max_size',
  ENABLE_NEW_ANIMATOR_FIBER = 'enable_new_animator_fiber',
  POST_DATA_BEFORE_UPDATE = 'post_data_before_update',
  ENABLE_REPORT_LIST_ITEM_LIFE_STATISTIC = 'enable_report_list_item_life_statistic',
  ENABLE_NATIVE_LIST = 'enable_native_list',
  ENABLE_ASYNC_DESTROY_ENGINE = 'enable_async_destroy_engine',
  CONCURRENT_LOOP_HIGH_PRIORITY_WORKER_COUNT_PERCENT = 'concurrent_loop_high_priority_worker_count_percent',
  ENABLE_DECOUPLED_LIST = 'enable_decoupled_list',
  DISABLE_LIST_CALLBACK_IF_DETACHED = 'disable_list_callback_if_detached',
  ENABLE_USE_CONTEXT_POOL = 'enable_use_context_pool',
  ENABLE_LIST_NEW_ARCHITECTURE = 'enable_list_new_architecture',
  FIX_LIST_WITH_SYNC_FLUSH = 'fix_list_with_sync_flush',
  FORCE_USE_SHADER_REPLACE_BLIT_WORKAROUND_CANVAS = 'force_use_shader_replace_blit_workaround_canvas',
  TRY_TO_FIX_NATIVE_WINDOW_LEAK_HARMONY_CANVAS = 'try_to_fix_native_window_leak_harmony_canvas',
  FIX_LIST_CALLBACK_LEAK_BUG = 'fix_list_callback_leak',
  ENABLE_HARMONY_NEW_IMAGE = 'enable_harmony_new_image',
  FIX_FILTER_DYNAMIC_UPDATE_BUG = 'fix_filter_dynamic_update_bug',
  // Temporary validation switch for the Fiber viewport + transition dynamic
  // update fix. Remove this key after the fix is verified online.
  FIX_FIBER_DYNAMIC_UPDATE_TRANSITION_CONSUME_BUG = 'fix_fiber_dynamic_update_transition_consume_bug',
  FIX_FIBER_REINSERT_DETACH_FROM_OLD_RENDER_PARENT = 'fix_fiber_reinsert_detach_from_old_render_parent',
  ENABLE_NEW_STYLING_PIPELINE = 'enable_new_styling_pipeline',
  ENABLE_ELEMENT_API_NEW_REGISTRATION = 'enable_element_api_new_registration',
}

/**
 * A singleton class for getting and caching trail values
 */
export class LynxTrailHub {
  private static instance: LynxTrailHub;
  private trailMap: Map<string, string> = new Map()

  public static getInstance(): LynxTrailHub {
    if (!LynxTrailHub.instance) {
      LynxTrailHub.instance = new LynxTrailHub();
    }
    return LynxTrailHub.instance;
  }

  public init(context: common.ApplicationContext): void {
    LynxServiceCenter.getService<ILynxTrailService>(LynxServiceType.Trail)?.initialize(context);
  }

  public stringValueForTrailKey(key: LynxTrailKey): string | undefined {
    let value = this.trailMap.get(key)
    LLog.i(TAG, 'stringValue: ' + value + ', forTrailKey: ' + key)
    return value
  }

  /**
   * setTrailMap to overwrite old values. ets values can't be read in asynchronous threads,
   * so the values must be put into the naive in advance.
   * @param trailValues
   */
  public setTrailMap(trailValues: Record<string, string>): void {
    LLog.i(TAG, 'setTrailMap: ' + JSON.stringify(trailValues))
    this.trailMap = new Map(Object.entries(trailValues))
    lynx.LynxTrailHubImpl.setTrailMap(trailValues);
  }
}
