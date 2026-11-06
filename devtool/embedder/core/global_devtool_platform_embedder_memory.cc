// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/global_devtool_platform_embedder_memory.h"

#include <algorithm>
#include <vector>

namespace lynx {
namespace devtool {

void EmbedderMemoryUsageRequest::Complete(const std::string& result,
                                          const std::string& error) {
  Callback callback;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    callback = std::move(callback_);
  }
  if (callback) callback(result, error);
}

Json::Value BuildEmbedderMemoryUsageResult(
    const tasm::performance::GlobalMemoryUsage& usage,
    int64_t collection_start_ms, int64_t collection_duration_ms,
    int64_t collection_timeout_ms, int64_t app_bytes) {
  Json::Value result(Json::objectValue);
  result["collectionStartMs"] = Json::Int64(collection_start_ms);
  // Completion describes the monitor query, not source completeness/freshness.
  result["collectionStatus"] = "completed";
  result["collectionDurationMs"] = Json::Int64(collection_duration_ms);
  result["collectionTimeoutMs"] = Json::Int64(collection_timeout_ms);
  result["expectedInstanceCount"] = Json::UInt64(usage.instances.size());
  result["completedInstanceCount"] = Json::UInt64(usage.instances.size());
  const auto& total = usage.total;
  result["totalBytes"] = Json::Int64(total.total_bytes);
  result["elementBytes"] = Json::Int64(total.element_bytes);
  result["elementNodeCount"] = total.element_count;
  result["viewBytes"] = Json::Int64(total.ui_bytes);
  result["mainThreadRuntimeBytes"] = Json::Int64(total.mts_bytes);
  result["backgroundThreadRuntimeBytes"] = Json::Int64(total.bts_bytes);
  app_bytes = std::max<int64_t>(0, app_bytes);
  result["appBytes"] = Json::Int64(app_bytes);
  result["ratioToApp"] = app_bytes > 0
                             ? static_cast<double>(total.total_bytes) /
                                   static_cast<double>(app_bytes)
                             : 0.0;

  std::vector<const tasm::performance::InstanceMemoryUsage*> sorted;
  sorted.reserve(usage.instances.size());
  for (const auto& instance : usage.instances) sorted.push_back(&instance);
  std::sort(sorted.begin(), sorted.end(),
            [](const auto* left, const auto* right) {
              if (left->page.total_bytes != right->page.total_bytes) {
                return left->page.total_bytes > right->page.total_bytes;
              }
              return left->instance_id < right->instance_id;
            });
  Json::Value instances(Json::arrayValue);
  for (const auto* source : sorted) {
    Json::Value instance(Json::objectValue);
    instance["instanceId"] = source->instance_id;
    instance["pageId"] = source->page_id;
    instance["url"] = source->url;
    instance["totalBytes"] = Json::Int64(source->page.total_bytes);
    instance["elementBytes"] = Json::Int64(source->page.element_bytes);
    instance["elementNodeCount"] = source->page.element_count;
    instance["viewBytes"] = Json::Int64(source->page.ui_bytes);
    instance["viewDetail"] = Json::Value(Json::objectValue);
    instance["mainThreadRuntimeBytes"] = Json::Int64(source->page.mts_bytes);
    // Page attribution and the whole shared VM heap are distinct quantities.
    instance["backgroundThreadRuntimeBytes"] =
        Json::Int64(source->page.bts_bytes);
    instance["btsHeapBytes"] = Json::Int64(source->bts_heap_bytes);
    instance["btsShared"] = source->page.bts_shared;
    instance["btsRuntimeGroupId"] = source->bts_runtime_group_id;
    instances.append(std::move(instance));
  }
  result["instances"] = std::move(instances);
  return result;
}

}  // namespace devtool
}  // namespace lynx
