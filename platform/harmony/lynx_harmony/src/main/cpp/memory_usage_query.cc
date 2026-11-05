// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/memory_usage_query.h"

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/algorithm.h"
#include "base/include/memory/process_memory_info.h"
#include "core/services/performance/memory_monitor/global_memory_monitor.h"

namespace lynx::harmony {
namespace {

struct QueryResult {
  tasm::performance::GlobalMemoryUsage usage;
  int64_t start_ms;
  int64_t duration_ms;
  int64_t timeout_ms;
  int64_t app_bytes;
};

class ObjectWriter {
 public:
  explicit ObjectWriter(napi_env env) : env_(env) {
    napi_create_object(env_, &object_);
  }

  napi_value Get() const { return object_; }

  void SetNumber(const char* key, int64_t value) {
    napi_value number = nullptr;
    napi_create_int64(env_, value, &number);
    napi_set_named_property(env_, object_, key, number);
  }

  void SetDouble(const char* key, double value) {
    napi_value number = nullptr;
    napi_create_double(env_, value, &number);
    napi_set_named_property(env_, object_, key, number);
  }

  void SetString(const char* key, const char* value,
                 size_t length = NAPI_AUTO_LENGTH) {
    napi_value string = nullptr;
    napi_create_string_utf8(env_, value, length, &string);
    napi_set_named_property(env_, object_, key, string);
  }

  void SetString(const char* key, const std::string& value) {
    SetString(key, value.data(), value.size());
  }

 private:
  napi_env env_;
  napi_value object_{nullptr};
};

napi_value CreateArray(napi_env env, napi_value object, const char* key,
                       size_t length) {
  napi_value array = nullptr;
  napi_create_array_with_length(env, length, &array);
  napi_set_named_property(env, object, key, array);
  return array;
}

void SetInstances(
    napi_env env, napi_value object, const char* key,
    const std::vector<tasm::performance::InstanceMemoryUsage>& sources,
    bool destroyed) {
  auto instances = CreateArray(env, object, key, sources.size());
  for (size_t i = 0; i < sources.size(); ++i) {
    const auto& source = sources[i];
    ObjectWriter instance(env);
    instance.SetNumber("instanceId", source.instance_id);
    instance.SetString("pageId", source.page_id);
    instance.SetString("url", source.url);
    instance.SetString("btsRuntimeGroupId", source.bts_runtime_group_id);
    instance.SetString("btsRuntimeType", source.bts_runtime_type);
    instance.SetNumber("btsRuntimeTotalBytes", source.bts_heap_bytes);
    if (destroyed) {
      instance.SetNumber("backgroundThreadRuntimeResidualBytes",
                         source.bts_residual_bytes);
    } else {
      instance.SetNumber("totalBytes", source.page.total_bytes);
      instance.SetNumber("elementBytes", source.page.element_bytes);
      instance.SetNumber("elementNodeCount", source.page.element_count);
      instance.SetNumber("viewBytes", source.page.ui_bytes);
      napi_value view_detail = nullptr;
      napi_create_object(env, &view_detail);
      napi_set_named_property(env, instance.Get(), "viewDetail", view_detail);
      instance.SetNumber("mainThreadRuntimeBytes", source.page.mts_bytes);
      instance.SetNumber("backgroundThreadRuntimeBytes", source.page.bts_bytes);
    }
    napi_set_element(env, instances, i, instance.Get());
  }
}

void DeliverResult(napi_env env, napi_value callback, void*, void* data) {
  std::unique_ptr<QueryResult> result(static_cast<QueryResult*>(data));
  // Environment teardown also calls this function to release queued data.
  if (!env || !callback) return;
  const auto& usage = result->usage;
  const bool completed =
      usage.status == tasm::performance::MemoryUsageQueryStatus::kOk;
  ObjectWriter writer(env);
  writer.SetNumber("collectionStartMs", result->start_ms);
  writer.SetString("collectionStatus", completed ? "completed" : "error");
  writer.SetString(
      "errorMessage",
      completed ? "" : tasm::performance::kMemoryMonitoringDisabledError);
  writer.SetNumber("collectionDurationMs", result->duration_ms);
  writer.SetNumber("collectionTimeoutMs", result->timeout_ms);
  writer.SetNumber("expectedInstanceCount", usage.instances.size());
  writer.SetNumber("completedInstanceCount", usage.instances.size());
  writer.SetNumber("totalBytes", usage.total.total_bytes);
  writer.SetNumber("appBytes", result->app_bytes > 0 ? result->app_bytes : 0);
  writer.SetDouble(
      "ratioToApp",
      result->app_bytes > 0
          ? static_cast<double>(usage.total.total_bytes) / result->app_bytes
          : 0.0);
  writer.SetNumber("elementBytes", usage.total.element_bytes);
  writer.SetNumber("elementNodeCount", usage.total.element_count);
  writer.SetNumber("viewBytes", usage.total.ui_bytes);
  writer.SetNumber("mainThreadRuntimeBytes", usage.total.mts_bytes);
  writer.SetNumber("backgroundThreadRuntimeBytes", usage.total.bts_bytes);
  SetInstances(env, writer.Get(), "instances", usage.instances, false);
  SetInstances(env, writer.Get(), "destroyedInstances",
               usage.destroyed_instances, true);
  napi_value receiver = nullptr;
  napi_get_undefined(env, &receiver);
  napi_value object = writer.Get();
  napi_call_function(env, receiver, callback, 1, &object, nullptr);
}

napi_value QueryGlobalMemoryUsage(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr, nullptr};
  napi_valuetype callback_type = napi_undefined;
  if (napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) != napi_ok ||
      argc < 1 || napi_typeof(env, args[0], &callback_type) != napi_ok ||
      callback_type != napi_function) {
    napi_throw_type_error(env, nullptr, "Callback required");
    return nullptr;
  }
  int64_t timeout_ms = 2000;
  if (argc > 1 && napi_get_value_int64(env, args[1], &timeout_ms) != napi_ok) {
    napi_throw_type_error(env, nullptr, "Invalid timeout");
    return nullptr;
  }
  if (timeout_ms <= 0) timeout_ms = 2000;
  napi_value name = nullptr;
  napi_create_string_utf8(env, "MemoryQuery", NAPI_AUTO_LENGTH, &name);
  napi_threadsafe_function function = nullptr;
  if (napi_create_threadsafe_function(env, args[0], nullptr, name, 1, 1,
                                      nullptr, nullptr, nullptr, DeliverResult,
                                      &function) != napi_ok) {
    napi_throw_error(env, nullptr, "Callback failed");
    return nullptr;
  }
  napi_unref_threadsafe_function(env, function);
  const auto start = std::chrono::steady_clock::now();
  const int64_t start_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  tasm::performance::GlobalMemoryMonitor::GetInstance().GetGlobalMemoryUsage(
      [function, start, start_ms,
       timeout_ms](tasm::performance::GlobalMemoryUsage usage) {
        base::InsertionSort(usage.instances.data(), usage.instances.size(),
                            [](const auto& left, const auto& right) {
                              return left.page.total_bytes >
                                     right.page.total_bytes;
                            });
        const int64_t duration_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start)
                .count();
        const int64_t app_bytes =
            usage.status == tasm::performance::MemoryUsageQueryStatus::kOk
                ? base::GetProcessPssBytes()
                : 0;
        // Construct the snapshot in place instead of moving a temporary again.
        std::unique_ptr<QueryResult> result(new QueryResult{
            std::move(usage), start_ms, duration_ms, timeout_ms, app_bytes});
        const auto status = napi_call_threadsafe_function(
            function, result.get(), napi_tsfn_nonblocking);
        if (status == napi_ok) result.release();
        if (status != napi_closing) {
          napi_release_threadsafe_function(function, napi_tsfn_release);
        }
      });
  napi_value result = nullptr;
  napi_get_undefined(env, &result);
  return result;
}

}  // namespace

void InitMemoryUsageQuery(napi_env env, napi_value exports) {
  napi_property_descriptor property = {"queryGlobalMemoryUsage",
                                       nullptr,
                                       QueryGlobalMemoryUsage,
                                       nullptr,
                                       nullptr,
                                       nullptr,
                                       napi_default,
                                       nullptr};
  napi_define_properties(env, exports, 1, &property);
}

}  // namespace lynx::harmony
