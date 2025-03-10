// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_LYNX_ENV_H_
#define CORE_RENDERER_UTILS_LYNX_ENV_H_

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "base/include/base_export.h"
#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"

namespace lynx {
namespace tasm {

class BASE_EXPORT_FOR_DEVTOOL LynxEnv {
 public:
  enum class Key : uint64_t {
    ENABLE_DEVTOOL = 0,
    DEVTOOL_COMPONENT_ATTACH,
    ENABLE_DEVTOOL_FOR_DEBUGGABLE_VIEW,
    ENABLE_LOGBOX,
    ENABLE_QUICKJS_CACHE,
    ANDROID_DISABLE_QUICKJS_CODE_CACHE,
    DISABLE_TRACING_GC,
    LAYOUT_PERFORMANCE_ENABLE,
    ENABLE_V8,
    ENABLE_PIPER_MONITOR,
    ENABLE_DOM_TREE,
    ENABLE_VSYNC_ALIGNED_FLUSH,
    ENABLE_VSYNC_ALIGNED_FLUSH_LOCAL,
    ENABLE_GLOBAL_FEATURE_SWITCH_STATISTIC,
    ENABLE_FEATURE_COUNTER,
    ENABLE_JSB_TIMING,
    ENABLE_ASYNC_JSB_TIMING,
    ENABLE_LONG_TASK_TIMING,
    DEVTOOL_CONNECTED,
    ENABLE_QUICKJS_DEBUG,
    ENABLE_TABLE_DEEP_CHECK,
    DISABLE_LEPUSNG_OPTIMIZE,
    FORCE_LYNX_NETWORK,
    FORCE_LYNX_NETWORK_ALLOWLIST,
    FORCE_LYNX_NETWORK_BLOCKLIST,
    TRAIL_ASYNC_HYDRATION,
    DISABLE_LAZY_CSS_DECODE,
    DECOUPLE_LAYOUTNODE_FROM_ELEMENT,
    REACT_LYNX_SETTINGS,
    TRAIL_REMOVE_COMPONENT_EXTRA_DATA,
    TRAIL_LYNX_RESOURCE_SERVICE_PROVIDER,
    ENABLE_COMPONENT_ASYNC_DECODE,
    V8_HEAP_SIZE,
    ENABLE_ASYNC_THREAD_CACHE,
    MULTI_TASM_THREAD_SIZE,
    MULTI_LAYOUT_THREAD_SIZE,
    ENABLE_FLUENCY_TRACE,
    GLOBAL_QUICK_CONTEXT_POOL_SIZE,
    ENABLE_REPORT_THREADED_ELEMENT_FLUSH_STATISTIC,
    STORAGE_DIR,
    VSYNC_TRIGGERED_FROM_UI_THREAD_ANDROID,
    CLIP_RADIUS_FLATTEN,
    ENABLE_UI_OP_BATCH,
    ENABLE_LAYOUT_ONLY_STATISTIC,
    VSYNC_POST_TASK_BY_EMERGENCY,
    ENABLE_USE_MAP_BUFFER_FOR_UI_PROPS,
    DISABLE_ONCE_DYNAMIC_CSS,
    ENABLE_REPORT_DYNAMIC_COMPONENT_EVENT,
    ENABLE_LAZY_IMPORT_CSS,
    BYTECODE_MAX_SIZE,
    ENABLE_NEW_ANIMATOR_FIBER,
    POST_DATA_BEFORE_UPDATE,
    ENABLE_REPORT_LIST_ITEM_LIFE_STATISTIC,
    ENABLE_NATIVE_LIST_NESTED,
    ENABLE_ASYNC_DESTROY_ENGINE,
    CONCURRENT_LOOP_HIGH_PRIORITY_WORKER_COUNT_PERCENT,
    ENABLE_USE_CONTEXT_POOL,
    ENABLE_FIBER_ELEMENT_FOR_RADON_DIFF,
    ENABLE_NATIVE_CREATE_VIEW_ASYNC,
    ENABLE_SIGNAL_API,
    // Please add new enum values above
    END_MARK,  // Keep this as the last enum value, and do not use
  };

 private:
  static inline const std::string& GetEnvKeyString(Key key) {
    static const base::NoDestructor<std::unordered_map<Key, std::string>>
        env_key_to_string_map({
            {Key::ENABLE_DEVTOOL, "enable_devtool"},
            {Key::DEVTOOL_COMPONENT_ATTACH, kLynxDevToolComponentAttach},
            {Key::ENABLE_DEVTOOL_FOR_DEBUGGABLE_VIEW,
             "enable_devtool_for_debuggable_view"},
            {Key::ENABLE_LOGBOX, kLynxEnableLogBox},
            {Key::ENABLE_QUICKJS_CACHE, "enable_quickjs_cache"},
            {Key::ANDROID_DISABLE_QUICKJS_CODE_CACHE,
             "ANDROID_DISABLE_QUICKJS_CODE_CACHE"},
            {Key::DISABLE_TRACING_GC, "DISABLE_TRACING_GC"},
            {Key::LAYOUT_PERFORMANCE_ENABLE, kLynxLayoutPerformanceEnable},
            {Key::ENABLE_V8, "enable_v8"},
            {Key::ENABLE_PIPER_MONITOR, kLynxEnablePiperMonitor},
            {Key::ENABLE_DOM_TREE, kLynxEnableDomTree},
            {Key::ENABLE_VSYNC_ALIGNED_FLUSH, "enable_vsync_aligned_flush"},
            {Key::ENABLE_VSYNC_ALIGNED_FLUSH_LOCAL,
             "enable_vsync_aligned_flush_local"},
            {Key::ENABLE_GLOBAL_FEATURE_SWITCH_STATISTIC,
             "ENABLE_GLOBAL_FEATURE_SWITCH_STATISTIC"},
            {Key::ENABLE_JSB_TIMING, "enable_jsb_timing"},
            {Key::ENABLE_FEATURE_COUNTER, "ENABLE_FEATURE_COUNTER"},
            {Key::ENABLE_ASYNC_JSB_TIMING, "enable_async_jsb_timing"},
            {Key::ENABLE_LONG_TASK_TIMING, "enable_long_task_timing"},
            {Key::DEVTOOL_CONNECTED, "devtool_connected"},
            {Key::STORAGE_DIR, "storage_dir"},
            {Key::ENABLE_QUICKJS_DEBUG, "enable_quickjs_debug"},
            {Key::ENABLE_TABLE_DEEP_CHECK, kLynxEnableTableDeepCheck},
            {Key::DISABLE_LEPUSNG_OPTIMIZE, "DISABLE_LEPUSNG_OPTIMIZE"},
            {Key::FORCE_LYNX_NETWORK,
             "FORCE_LYNX_NETWORK"},  // These functions using settings will be
                                     // removed after we fully switch to Lynx
                                     // Network.
            {Key::FORCE_LYNX_NETWORK_ALLOWLIST, "FORCE_LYNX_NETWORK_ALLOWLIST"},
            {Key::FORCE_LYNX_NETWORK_BLOCKLIST, "FORCE_LYNX_NETWORK_BLOCKLIST"},
            {Key::TRAIL_ASYNC_HYDRATION, "trail_async_hydration"},
            {Key::DISABLE_LAZY_CSS_DECODE, "disable_lazy_css_decode"},
            {Key::DECOUPLE_LAYOUTNODE_FROM_ELEMENT,
             "decouple_layoutnode_from_element"},
            {Key::TRAIL_REMOVE_COMPONENT_EXTRA_DATA,
             "trialRemoveComponentExtraData"},
            {Key::REACT_LYNX_SETTINGS, "lynxsdk_react_lynx_settings"},
            {Key::TRAIL_LYNX_RESOURCE_SERVICE_PROVIDER,
             "trialLynxResourceServiceProvider"},
            {Key::ENABLE_COMPONENT_ASYNC_DECODE, "enableComponentAsyncDecode"},
            {Key::V8_HEAP_SIZE, "V8_HEAP_SIZE"},
            {Key::ENABLE_ASYNC_THREAD_CACHE, "enable_async_thread_cache"},
            {Key::MULTI_TASM_THREAD_SIZE, "multi_tasm_thread_size"},
            {Key::MULTI_LAYOUT_THREAD_SIZE, "multi_layout_thread_size"},
            {Key::ENABLE_FLUENCY_TRACE, "ENABLE_FLUENCY_TRACE"},
            {Key::ENABLE_REPORT_THREADED_ELEMENT_FLUSH_STATISTIC,
             "enable_report_threaded_element_flush_statistic"},
            {Key::GLOBAL_QUICK_CONTEXT_POOL_SIZE,
             "global_quick_context_pool_size"},
            {Key::CLIP_RADIUS_FLATTEN, "clip_radius_flatten"},
            {Key::ENABLE_UI_OP_BATCH, "enable_ui_op_batch"},
            {Key::ENABLE_LAYOUT_ONLY_STATISTIC, "enable_layout_only_statistic"},
            {Key::VSYNC_TRIGGERED_FROM_UI_THREAD_ANDROID,
             "lynx_vsync_triggered_from_ui_thread_android"},
            {Key::VSYNC_POST_TASK_BY_EMERGENCY,
             "lynx_vsync_post_task_by_emergency"},
            {Key::ENABLE_USE_MAP_BUFFER_FOR_UI_PROPS,
             "use_map_buffer_for_ui_props"},
            {Key::ENABLE_LAZY_IMPORT_CSS, "enable_lazy_import_css"},
            {Key::DISABLE_ONCE_DYNAMIC_CSS, "disable_once_dynamic_css"},
            {Key::ENABLE_REPORT_DYNAMIC_COMPONENT_EVENT,
             "enable_report_dynamic_component_event"},
            {Key::BYTECODE_MAX_SIZE, "bytecode_max_size"},
            {Key::ENABLE_NEW_ANIMATOR_FIBER, "enable_new_animator_fiber"},
            {Key::POST_DATA_BEFORE_UPDATE, "post_data_before_update"},
            {Key::ENABLE_REPORT_LIST_ITEM_LIFE_STATISTIC,
             "enable_report_list_item_life_statistic"},
            {Key::ENABLE_NATIVE_LIST_NESTED, "enable_native_list_nested"},
            {Key::ENABLE_ASYNC_DESTROY_ENGINE, "enable_async_destroy_engine"},
            {Key::CONCURRENT_LOOP_HIGH_PRIORITY_WORKER_COUNT_PERCENT,
             "concurrent_loop_high_priority_worker_count_percent"},
            {Key::ENABLE_USE_CONTEXT_POOL, "enable_use_context_pool"},
            {Key::ENABLE_FIBER_ELEMENT_FOR_RADON_DIFF,
             "enable_fiber_element_for_radon_diff"},
            {Key::ENABLE_NATIVE_CREATE_VIEW_ASYNC,
             "enable_native_create_view_async"},
            {Key::ENABLE_SIGNAL_API, "enable_signal_api"},
        });
    auto it = (*env_key_to_string_map).find(key);
    DCHECK(it != (*env_key_to_string_map).end());
    return it->second;
  }

  enum class EnvType : int32_t { LOCAL, EXTERNAL };

 public:
  constexpr static const char* const kLynxLayoutPerformanceEnable =
      "layout_performance_enable";
  constexpr static const char* const kLynxDevToolComponentAttach =
      "devtool_component_attach";
  constexpr static const char* const kLynxDevToolEnable = "enable_devtool";
  constexpr static const char* const kLynxEnableDomTree = "enable_dom_tree";
  constexpr static const char* const kLynxEnableTableDeepCheck =
      "enable_table_deep_check";
  constexpr static const char* const kLynxEnableLogBox = "enable_logbox";
  constexpr static const char* const kLynxEnablePiperMonitor =
      "enablePiperMonitor";

  constexpr static const char* const kLocalEnvValueTrue = "1";
  constexpr static const char* const kLocalEnvValueFalse = "0";

  static LynxEnv& GetInstance();
  static void onPiperInvoked(const std::string& module_name,
                             const std::string& method_name,
                             const std::string& param_str,
                             const std::string& url,
                             const std::string& invoke_session);
  static void onPiperResponsed(const std::string& method_name,
                               const std::string& module_name,
                               const std::string& url,
                               const std::string& response,
                               const std::string& invoke_session);

  // env
  void CleanExternalCache();
  void SetBoolLocalEnv(const std::string& key, bool value);
  void SetLocalEnv(const std::string& key, const std::string& value);
  void SetEnvMask(const std::string& key, bool value);
  bool GetBoolEnv(Key key, bool default_value,
                  EnvType type = EnvType::EXTERNAL);
  long GetLongEnv(Key key, int default_value, EnvType type = EnvType::EXTERNAL);
  std::optional<std::string> GetStringEnv(Key key,
                                          EnvType type = EnvType::EXTERNAL);
  std::string GetDebugDescription();

  // group env, for devtool
  void SetGroupedEnv(const std::string& key, bool value,
                     const std::string& group_key);
  void SetGroupedEnv(const std::unordered_set<std::string>& new_group_values,
                     const std::string& group_key);
  std::unordered_set<std::string> GetGroupedEnv(const std::string& group_key);
  inline std::string GetStorageDirectory() {
    return GetLocalEnv(Key::STORAGE_DIR).value_or("");
  }
  inline void SetStorageDirectory(const std::string& path) {
    SetLocalEnv(GetEnvKeyString(Key::STORAGE_DIR), path);
  }

  bool IsDevToolComponentAttach();
  bool IsDevToolEnabled();
  bool IsDevToolEnabledForDebuggableView();
  bool IsLogBoxEnabled();
  bool IsQuickjsCacheEnabled();
  bool IsDisableTracingGC();
  bool IsLayoutPerformanceEnabled();
  long GetV8Enabled();
  bool IsPiperMonitorEnabled();
  bool IsDomTreeEnabled();
  bool IsDevToolConnected();
  bool IsQuickjsDebugEnabled();
  bool IsJsDebugEnabled(bool force_use_lightweight_js_engine);
  bool IsTableDeepCheckEnabled();
  bool IsDisabledLepusngOptimize();
  bool GetVsyncAlignedFlushGlobalSwitch();
  bool EnableGlobalFeatureSwitchStatistic();
  bool EnableFeatureCounter();
  bool EnableJSBTiming();
  bool EnableAsyncJSBTiming();
  bool EnableLongTaskTiming();
  int64_t GetV8HeapSize();
  std::unordered_set<std::string> GetActivatedCDPDomains();
  bool IsDebugModeEnabled();
  int32_t GetGlobalQuickContextPoolSize(int32_t default_value);
  bool EnableUIOpBatch();
  bool EnableCSSLazyImport();
  bool EnableNewAnimatorFiber();
  bool IsVSyncTriggeredInUiThreadAndroid();
  bool IsVSyncPostTaskByEmergency();
  bool EnableUseMapBufferForUIProps();
  bool EnablePostDataBeforeUpdateTemplate();
  bool EnableReportListItemLifeStatistic();
  bool EnableNativeListNested();
  bool EnableAsyncDestroyEngine();
  bool EnableComponentAsyncDecode();
  bool EnableUseContextPool();
  bool EnableNativeCreateViewAsync();
  bool EnableSignalAPI();

  LynxEnv(const LynxEnv&) = delete;
  LynxEnv& operator=(const LynxEnv&) = delete;
  LynxEnv(LynxEnv&&) = delete;
  LynxEnv& operator=(LynxEnv&&) = delete;

 private:
  LynxEnv() = default;

  bool GetEnvMask(Key key);
  std::optional<std::string> GetLocalEnv(Key key);
  std::optional<std::string> GetExternalEnv(Key key);

  friend class base::NoDestructor<LynxEnv>;

  std::mutex mutex_;
  std::unordered_map<std::string, std::string> local_env_map_;
  std::unordered_map<std::string, bool> env_mask_map_;
  std::unordered_map<std::string, std::unordered_set<std::string>>
      env_group_sets_;

  std::recursive_mutex external_env_mutex_;
  std::unordered_map<Key, std::string> external_env_map_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_LYNX_ENV_H_
