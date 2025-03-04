// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/lynx_env.h"

#include <algorithm>
#include <cstdbool>
#include <unordered_map>
#include <utility>

#include "core/renderer/utils/lynx_trail_hub.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

#if OS_ANDROID
#include "core/renderer/utils/android/lynx_env_android.h"
#endif

namespace lynx {
namespace tasm {

LynxEnv& LynxEnv::GetInstance() {
  static base::NoDestructor<LynxEnv> instance;
  return *instance;
}

void LynxEnv::onPiperInvoked(const std::string& module_name,
                             const std::string& method_name,
                             const std::string& param_str,
                             const std::string& url,
                             const std::string& invoke_session) {
#if OS_ANDROID
  tasm::LynxEnvAndroid::onPiperInvoked(module_name, method_name, param_str,
                                       url);
#endif
}

void LynxEnv::onPiperResponsed(const std::string& module_name,
                               const std::string& method_name,
                               const std::string& url,
                               const std::string& response,
                               const std::string& invoke_session) {
#if OS_ANDROID
#endif
}

void LynxEnv::SetBoolLocalEnv(const std::string& key, bool value) {
  SetLocalEnv(key, value ? kLocalEnvValueTrue : kLocalEnvValueFalse);
}

void LynxEnv::SetLocalEnv(const std::string& key, const std::string& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto old_value = local_env_map_.find(key);
  if (old_value != local_env_map_.end()) {
    local_env_map_.erase(old_value);
  }
  local_env_map_.emplace(key, value);
}

void LynxEnv::SetGroupedEnv(const std::string& key, bool value,
                            const std::string& group_key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = env_group_sets_.find(group_key);
  if (it == env_group_sets_.end()) {
    std::unordered_set<std::string> new_set;
    it = env_group_sets_.insert(it, std::make_pair(group_key, new_set));
  }
  if (value) {
    it->second.insert(key);
  } else {
    it->second.erase(key);
  }
}

void LynxEnv::SetGroupedEnv(
    const std::unordered_set<std::string>& new_group_values,
    const std::string& group_key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto old_value = env_group_sets_.find(group_key);
  if (old_value != env_group_sets_.end()) {
    env_group_sets_.erase(old_value);
  }
  env_group_sets_.emplace(group_key, new_group_values);
}

long LynxEnv::GetLongEnv(Key key, int default_value, EnvType type) {
  std::optional<std::string> string_result = GetStringEnv(key, type);
  if (!string_result.has_value() || (*string_result).empty()) {
    return default_value;
  }
  char* end_tag;
  long result = std::strtol((*string_result).c_str(), &end_tag, 10);
  if (*end_tag != '\0') {
    return default_value;
  }
  return result;
}

bool LynxEnv::GetBoolEnv(Key key, bool default_value, EnvType type) {
  std::optional<std::string> string_result = GetStringEnv(key, type);
  if (!string_result.has_value() || (*string_result).empty()) {
    return default_value;
  }

  std::string string = *string_result;
  if (string == kLocalEnvValueTrue) {
    return true;
  } else if (string == kLocalEnvValueFalse) {
    return false;
  } else {
    static const std::string true_result = "true";
    return string.size() == true_result.size() &&
           std::equal(string.begin(), string.end(), true_result.begin(),
                      [](char a, char b) {
                        return std::tolower(a) == std::tolower(b);
                      });
  }
}

std::optional<std::string> LynxEnv::GetStringEnv(Key key, EnvType type) {
  std::optional<std::string> result = std::nullopt;
  switch (type) {
    case EnvType::EXTERNAL: {
      result = GetExternalEnv(key);
    } break;
    case EnvType::LOCAL: {
      result = GetLocalEnv(key);
    } break;
    default: {
      DCHECK(false);
    } break;
  }
  return result;
}

std::string LynxEnv::GetDebugDescription() {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  writer.StartObject();
  for (Key key = (Key)0; key < Key::END_MARK;) {
    std::string key_string = GetEnvKeyString(key);
    std::optional<std::string> value = GetStringEnv(key);
    if (value.has_value()) {
      writer.Key(key_string.c_str());
      writer.String((*value).c_str());
    }
    key = (Key)((uint64_t)key + 1);
  }
  writer.EndObject();
  std::string result = buffer.GetString();
  return result;
}

void LynxEnv::SetEnvMask(const std::string& key, bool value) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto old_value = env_mask_map_.find(key);
  if (old_value != env_mask_map_.end()) {
    env_mask_map_.erase(old_value);
  }
  env_mask_map_.emplace(key, value);
}

bool LynxEnv::GetEnvMask(Key key) {
  std::string key_string = GetEnvKeyString(key);
  auto value = env_mask_map_.find(key_string);
  return value != env_mask_map_.end() ? (*value).second : true;
}

std::unordered_set<std::string> LynxEnv::GetGroupedEnv(
    const std::string& group_key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = env_group_sets_.find(group_key);
  if (it != env_group_sets_.end()) {
    return it->second;
  }
  return std::unordered_set<std::string>();
}

bool LynxEnv::IsDevToolComponentAttach() {
  return GetBoolEnv(Key::DEVTOOL_COMPONENT_ATTACH, false, EnvType::LOCAL);
}

bool LynxEnv::IsDevToolEnabled() {
  return GetBoolEnv(Key::ENABLE_DEVTOOL, false, EnvType::LOCAL);
}

bool LynxEnv::IsDevToolEnabledForDebuggableView() {
  return GetBoolEnv(Key::ENABLE_DEVTOOL_FOR_DEBUGGABLE_VIEW, false,
                    EnvType::LOCAL);
}

bool LynxEnv::IsLogBoxEnabled() {
  return IsDevToolComponentAttach() &&
         GetBoolEnv(Key::ENABLE_LOGBOX, true, EnvType::LOCAL);
}

bool LynxEnv::IsQuickjsCacheEnabled() {
  bool enable_quickjs_cache =
      GetBoolEnv(Key::ENABLE_QUICKJS_CACHE, true, EnvType::LOCAL);
  if (!enable_quickjs_cache) {
    return false;
  }

  std::optional<std::string> disable_quickjs_bytecode =
      GetLocalEnv(Key::ANDROID_DISABLE_QUICKJS_CODE_CACHE);
  if (!disable_quickjs_bytecode.has_value()) {
    disable_quickjs_bytecode =
        GetExternalEnv(Key::ANDROID_DISABLE_QUICKJS_CODE_CACHE);
  }
  return disable_quickjs_bytecode != kLocalEnvValueTrue &&
         disable_quickjs_bytecode != "true";
}

bool LynxEnv::IsDisableTracingGC() {
  return GetBoolEnv(Key::DISABLE_TRACING_GC, false);
}

bool LynxEnv::IsLayoutPerformanceEnabled() {
  return GetBoolEnv(Key::LAYOUT_PERFORMANCE_ENABLE, false, EnvType::LOCAL);
}

bool LynxEnv::IsPiperMonitorEnabled() {
  return GetBoolEnv(Key::ENABLE_PIPER_MONITOR, false, EnvType::LOCAL);
}

bool LynxEnv::IsDomTreeEnabled() {
  return (IsDevToolEnabled() || IsDevToolEnabledForDebuggableView()) &&
         GetBoolEnv(Key::ENABLE_DOM_TREE, true, EnvType::LOCAL);
}

bool LynxEnv::GetVsyncAlignedFlushGlobalSwitch() {
  return GetBoolEnv(Key::ENABLE_VSYNC_ALIGNED_FLUSH_LOCAL, true,
                    EnvType::LOCAL);
}

bool LynxEnv::EnableGlobalFeatureSwitchStatistic() {
  return GetBoolEnv(Key::ENABLE_GLOBAL_FEATURE_SWITCH_STATISTIC, false);
}

bool LynxEnv::EnableFeatureCounter() {
  return GetBoolEnv(Key::ENABLE_FEATURE_COUNTER, false);
}

bool LynxEnv::EnableJSBTiming() {
  return GetBoolEnv(Key::ENABLE_JSB_TIMING, false);
}

bool LynxEnv::EnableAsyncJSBTiming() {
  return GetBoolEnv(Key::ENABLE_ASYNC_JSB_TIMING, false);
}

bool LynxEnv::EnableLongTaskTiming() {
  return GetBoolEnv(Key::ENABLE_LONG_TASK_TIMING, false);
}

bool LynxEnv::IsDevToolConnected() {
  return GetBoolEnv(Key::DEVTOOL_CONNECTED, false, EnvType::LOCAL);
}

bool LynxEnv::IsTableDeepCheckEnabled() {
  return GetBoolEnv(Key::ENABLE_TABLE_DEEP_CHECK, false, EnvType::LOCAL);
}

bool LynxEnv::IsDisabledLepusngOptimize() {
  return GetBoolEnv(Key::DISABLE_LEPUSNG_OPTIMIZE, false);
}

std::unordered_set<std::string> LynxEnv::GetActivatedCDPDomains() {
  return GetGroupedEnv("activated_cdp_domains");
}

bool LynxEnv::IsDebugModeEnabled() {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE || ENABLE_TESTBENCH_RECORDER
  return true;
#else
  return false;
#endif
}

std::optional<std::string> LynxEnv::GetLocalEnv(Key key) {
  std::string key_string = GetEnvKeyString(key);
  std::lock_guard<std::mutex> lock(mutex_);
  if (local_env_map_.count(key_string) > 0) {
    bool mask = GetEnvMask(key);
    return mask ? local_env_map_[key_string] : kLocalEnvValueFalse;
  }
  return std::nullopt;
}

int64_t LynxEnv::GetV8HeapSize() {
  return GetLongEnv(Key::V8_HEAP_SIZE, 0, EnvType::EXTERNAL);
}

std::optional<std::string> LynxEnv::GetExternalEnv(Key key) {
  std::lock_guard<std::recursive_mutex> lock(external_env_mutex_);
  auto env_it = external_env_map_.find(key);
  if (env_it != external_env_map_.end()) {
    return env_it->second;
  }

  const std::string& key_string = GetEnvKeyString(key);
  std::optional<std::string> string_value =
      LynxTrailHub::GetInstance().GetStringForTrailKey(key_string);
  if (string_value.has_value()) {
    external_env_map_.emplace(key, *string_value);
  }
  return string_value;
}

void LynxEnv::CleanExternalCache() {
  std::lock_guard<std::recursive_mutex> lock(external_env_mutex_);
  external_env_map_.clear();
}

int32_t LynxEnv::GetGlobalQuickContextPoolSize(int32_t default_value) {
  return static_cast<int32_t>(GetLongEnv(Key::GLOBAL_QUICK_CONTEXT_POOL_SIZE,
                                         default_value, EnvType::EXTERNAL));
}

bool LynxEnv::EnableUIOpBatch() {
  return GetBoolEnv(Key::ENABLE_UI_OP_BATCH, false);
}

bool LynxEnv::EnableCSSLazyImport() {
  static bool cached_enable_lazy_import_css_result =
      GetBoolEnv(Key::ENABLE_LAZY_IMPORT_CSS, false);
  return cached_enable_lazy_import_css_result;
}

bool LynxEnv::EnableNewAnimatorFiber() {
  return GetBoolEnv(Key::ENABLE_NEW_ANIMATOR_FIBER, true);
}

bool LynxEnv::IsVSyncTriggeredInUiThreadAndroid() {
  return GetBoolEnv(Key::VSYNC_TRIGGERED_FROM_UI_THREAD_ANDROID, false);
}

bool LynxEnv::IsVSyncPostTaskByEmergency() {
  return GetBoolEnv(Key::VSYNC_POST_TASK_BY_EMERGENCY, false);
}

bool LynxEnv::EnableUseMapBufferForUIProps() {
  return GetBoolEnv(Key::ENABLE_USE_MAP_BUFFER_FOR_UI_PROPS, false);
}

bool LynxEnv::EnablePostDataBeforeUpdateTemplate() {
  return GetBoolEnv(Key::POST_DATA_BEFORE_UPDATE, true);
}

bool LynxEnv::EnableReportListItemLifeStatistic() {
  return GetBoolEnv(Key::ENABLE_REPORT_LIST_ITEM_LIFE_STATISTIC, false);
}

bool LynxEnv::EnableNativeListNested() {
  return GetBoolEnv(Key::ENABLE_NATIVE_LIST_NESTED, true);
}

bool LynxEnv::EnableAsyncDestroyEngine() {
  return GetBoolEnv(Key::ENABLE_ASYNC_DESTROY_ENGINE, false);
}

bool LynxEnv::EnableComponentAsyncDecode() {
  return GetBoolEnv(Key::ENABLE_COMPONENT_ASYNC_DECODE, false);
}

bool LynxEnv::EnableUseContextPool() {
  // TODO(zhoupeng.z): remove this trail option on SDK version 3.2
  return GetBoolEnv(Key::ENABLE_USE_CONTEXT_POOL, true);
}

bool LynxEnv::EnableNativeCreateViewAsync() {
  return GetBoolEnv(Key::ENABLE_NATIVE_CREATE_VIEW_ASYNC, false);
}

bool LynxEnv::EnableSignalAPI() {
  return GetBoolEnv(Key::ENABLE_SIGNAL_API, false);
}
}  // namespace tasm
}  // namespace lynx
