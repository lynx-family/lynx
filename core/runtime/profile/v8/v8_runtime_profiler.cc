// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/profile/v8/v8_runtime_profiler.h"

#if ENABLE_TRACE_PERFETTO
#include <memory>
#include <string>
#include <utility>

#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace profile {

V8RuntimeProfiler::V8RuntimeProfiler(
    const std::shared_ptr<V8RuntimeProfilerWrapper>& impl)
    : impl_(impl) {}

V8RuntimeProfiler::~V8RuntimeProfiler() { impl_ = nullptr; }

void V8RuntimeProfiler::StartProfiling(bool is_create) {
  auto task = [impl = impl_] {
    if (impl) {
      impl->StartProfiling();
    }
  };

  RuntimeProfiler::StartProfiling(std::move(task), is_create);
}

std::unique_ptr<RuntimeProfile> V8RuntimeProfiler::StopProfiling(
    bool is_destory) {
  std::string runtime_profile = "";
  auto task = [impl = impl_, &runtime_profile] {
    if (impl) {
      std::shared_ptr<V8CpuProfile> cpu_profile = impl->StopProfiling();
      if (cpu_profile) {
        rapidjson::Document doc(rapidjson::kObjectType);
        auto& allocator = doc.GetAllocator();
        rapidjson::Document profile(rapidjson::kObjectType);
        int64_t last_timestamp = cpu_profile->start_timestamp;
        profile.AddMember("startTime", last_timestamp, allocator);
        profile.AddMember("endTime", cpu_profile->end_timestamp, allocator);
        rapidjson::Document samples(rapidjson::kArrayType);
        rapidjson::Document timeDeltas(rapidjson::kArrayType);
        auto sample_count = cpu_profile->samples.size();
        for (size_t i = 0; i < sample_count; i++) {
          auto node_id = cpu_profile->samples[i];
          auto time_delta = cpu_profile->time_deltas[i];
          samples.PushBack(node_id, allocator);
          timeDeltas.PushBack(time_delta, allocator);
        }
        profile.AddMember("samples", samples, allocator);
        profile.AddMember("timeDeltas", timeDeltas, allocator);
        rapidjson::Document nodes(rapidjson::kArrayType);
        auto node_count = cpu_profile->nodes.size();
        for (size_t i = 0; i < node_count; i++) {
          const V8CpuProfileNode& profile_node = cpu_profile->nodes[i];
          rapidjson::Document node(rapidjson::kObjectType);
          node.AddMember("id", profile_node.id, allocator);
          rapidjson::Document callFrame(rapidjson::kObjectType);
          callFrame.AddMember("functionName",
                              profile_node.call_frame.function_name, allocator);
          callFrame.AddMember("url", profile_node.call_frame.url, allocator);
          callFrame.AddMember("lineNumber", profile_node.call_frame.line_number,
                              allocator);
          callFrame.AddMember("columnNumber",
                              profile_node.call_frame.column_number, allocator);
          node.AddMember("callFrame", callFrame, allocator);
          rapidjson::Document children(rapidjson::kArrayType);
          auto child_count = profile_node.children.size();
          for (size_t i = 0; i < child_count; i++) {
            children.PushBack(profile_node.children[i], allocator);
          }
          node.AddMember("children", children, allocator);
          nodes.PushBack(node, allocator);
        }
        profile.AddMember("nodes", nodes, allocator);
        doc.AddMember("profile", profile, allocator);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        runtime_profile = buffer.GetString();
      }
    }
  };

  RuntimeProfiler::StopProfiling(std::move(task), is_destory);

  if (!runtime_profile.empty()) {
    return std::make_unique<RuntimeProfile>(runtime_profile, track_id_);
  }
  return nullptr;
}

void V8RuntimeProfiler::SetupProfiling(int32_t sampling_interval) {
  auto task = [impl = impl_, sampling_interval] {
    if (impl) {
      impl->SetupProfiling(sampling_interval);
    }
  };

  RuntimeProfiler::SetupProfiling(std::move(task));
}

trace::RuntimeProfilerType V8RuntimeProfiler::GetType() {
  return trace::RuntimeProfilerType::v8;
}

}  // namespace profile
}  // namespace lynx

#endif
