// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/profile/v8/v8_runtime_profiler_wrapper_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "core/runtime/jsi/v8/v8_helper.h"

namespace lynx {
namespace profile {

std::shared_ptr<V8RuntimeProfilerWrapperImpl>
V8RuntimeProfilerWrapperImpl::GetInstance() {
  static thread_local std::shared_ptr<V8RuntimeProfilerWrapperImpl> instance =
      std::make_shared<V8RuntimeProfilerWrapperImpl>();
  return instance;
}

V8RuntimeProfilerWrapperImpl::V8RuntimeProfilerWrapperImpl()
    : cpu_profiler_(nullptr), profiling_count_(0), is_inited_(false) {}

V8RuntimeProfilerWrapperImpl::~V8RuntimeProfilerWrapperImpl() {
  if (cpu_profiler_) {
    cpu_profiler_->Dispose();
    cpu_profiler_ = nullptr;
    title_.Clear();
  }
  v8_isolate_.reset();
}

void V8RuntimeProfilerWrapperImpl::Initialize(
    std::shared_ptr<piper::V8IsolateInstance> vm) {
  if (is_inited_) {
    return;
  }
  if (cpu_profiler_) {
    cpu_profiler_->Dispose();
    cpu_profiler_ = nullptr;
    title_.Clear();
  }
  v8_isolate_ = vm;
  auto isolate = vm->Isolate();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  cpu_profiler_ = v8::CpuProfiler::New(isolate);
  title_ = piper::detail::V8Helper::ConvertToV8String(isolate, "v8");
  is_inited_ = true;
}

void V8RuntimeProfilerWrapperImpl::StartProfiling() {
  if (cpu_profiler_) {
    if (profiling_count_ == 0) {
      // One thread only need start once.
      cpu_profiler_->StartProfiling(title_, true);
    }
    profiling_count_++;
  }
}

std::unique_ptr<V8CpuProfile> V8RuntimeProfilerWrapperImpl::StopProfiling() {
  if (cpu_profiler_ == nullptr || profiling_count_ <= 0) {
    // CpuProfiler hasn't Inited or Doesn't have JSContext is profiling
    return nullptr;
  }

  profiling_count_--;
  if (profiling_count_ != 0) {
    // current thread still have JSContext is profiling
    return nullptr;
  }
  v8::CpuProfile* cpu_profile = cpu_profiler_->StopProfiling(title_);
  if (cpu_profile) {
    std::unique_ptr<V8CpuProfile> runtime_profile =
        std::make_unique<V8CpuProfile>();
    uint64_t last_timestamp = cpu_profile->GetStartTime();
    runtime_profile->start_timestamp = last_timestamp;
    runtime_profile->end_timestamp = cpu_profile->GetEndTime();
    auto sample_count = cpu_profile->GetSamplesCount();
    // In order to compressing profile data size, doesn't record sampled node
    // id and time until the call stack changed.
    uint32_t last_node_id = 0;
    for (int i = 0; i < sample_count; i++) {
      auto node = cpu_profile->GetSample(i);
      auto node_id = node->GetNodeId();
      if (node_id == last_node_id) {
        continue;
      }
      runtime_profile->samples.emplace_back(node_id);
      auto sample_time = cpu_profile->GetSampleTimestamp(i);
      runtime_profile->time_deltas.emplace_back(sample_time - last_timestamp);
      last_node_id = node_id;
      last_timestamp = sample_time;
    }
    FlattenProfileNodes(runtime_profile->nodes, cpu_profile->GetTopDownRoot());
    cpu_profile->Delete();
    return runtime_profile;
  }
  return nullptr;
}

void V8RuntimeProfilerWrapperImpl::SetupProfiling(int32_t sampling_interval) {
  if (cpu_profiler_) {
    cpu_profiler_->SetSamplingInterval(static_cast<int>(sampling_interval));
  }
}

void V8RuntimeProfilerWrapperImpl::FlattenProfileNodes(
    std::vector<V8CpuProfileNode>& nodes,
    const v8::CpuProfileNode* profile_node) {
  if (profile_node == nullptr) {
    return;
  }

  V8CpuProfileNode node;
  node.id = profile_node->GetNodeId();
  node.call_frame.function_name = profile_node->GetFunctionNameStr();
  node.call_frame.url = profile_node->GetScriptResourceNameStr();
  // ref:
  // https://chromedevtools.github.io/devtools-protocol/tot/Runtime/#type-CallFrame
  // CallFrame lineNumber and columnNumber is 0-based, v8::CpuProfileNode is
  // 1-based.
  node.call_frame.line_number = profile_node->GetLineNumber() - 1;
  node.call_frame.column_number = profile_node->GetColumnNumber() - 1;
  node.call_frame.script_id = std::to_string(profile_node->GetScriptId());

  auto child_count = profile_node->GetChildrenCount();
  for (int i = 0; i < child_count; i++) {
    auto child_node = profile_node->GetChild(i);
    node.children.emplace_back(child_node->GetNodeId());
    FlattenProfileNodes(nodes, child_node);
  }
  nodes.emplace_back(std::move(node));
}

}  // namespace profile
}  // namespace lynx
