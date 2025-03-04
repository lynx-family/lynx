// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_PROFILE_V8_V8_RUNTIME_PROFILER_WRAPPER_H_
#define CORE_RUNTIME_PROFILE_V8_V8_RUNTIME_PROFILER_WRAPPER_H_

#include <memory>
#include <string>
#include <vector>

namespace lynx {
namespace profile {

struct V8CpuProfileCallFrame {
  // JavaScript function name
  std::string function_name;
  // JavaScript script id
  std::string script_id;
  // JavaScript script url
  std::string url;
  // JavaScript script line number
  int32_t line_number;
  // JavaScript script column number
  int32_t column_number;
};

struct V8CpuProfileNode {
  // Unique id of the node
  int32_t id;
  // Function info
  V8CpuProfileCallFrame call_frame;
  // Number of samples that this node was on top of the call stack
  int32_t hit_count;
  // Children node ids
  std::vector<int32_t> children;
};

struct V8CpuProfile {
  // Profiling start time in microseconds
  int64_t start_timestamp;
  // Profiling end time in microseconds
  int64_t end_timestamp;
  // Profile nodes
  std::vector<V8CpuProfileNode> nodes;
  // Stack top node id
  std::vector<int32_t> samples;
  // Time delta between adjacent samples in microseconds.
  std::vector<int64_t> time_deltas;
};

class V8RuntimeProfilerWrapper {
 public:
  V8RuntimeProfilerWrapper() = default;
  virtual ~V8RuntimeProfilerWrapper() = default;
  virtual void StartProfiling() = 0;
  virtual std::unique_ptr<V8CpuProfile> StopProfiling() = 0;
  virtual void SetupProfiling(int32_t sampling_interval) = 0;
};

}  // namespace profile
}  // namespace lynx

#endif  // CORE_RUNTIME_PROFILE_V8_V8_RUNTIME_PROFILER_WRAPPER_H_
