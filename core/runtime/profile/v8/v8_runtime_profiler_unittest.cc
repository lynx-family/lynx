// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/profile/v8/v8_runtime_profiler.h"

#include <memory>

#include "core/runtime/profile/v8/v8_runtime_profiler_wrapper.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace profile {
namespace testing {

class V8RuntimeProfilerTest : public V8RuntimeProfilerWrapper {
 public:
  V8RuntimeProfilerTest() : profiling_count_(0) {}
  virtual ~V8RuntimeProfilerTest() = default;
  virtual void StartProfiling() override;
  virtual std::unique_ptr<V8CpuProfile> StopProfiling() override;
  virtual void SetupProfiling(int32_t sampling_interval) override;
  uint32_t profiling_count_;
};

void V8RuntimeProfilerTest::StartProfiling() {
  profiling_count_++;
  printf("StartProfiling: %d\n", profiling_count_);
}

void V8RuntimeProfilerTest::SetupProfiling(int32_t sampling_interval) {
  printf("SetupProfiling: %d\n", sampling_interval);
}

std::unique_ptr<V8CpuProfile> V8RuntimeProfilerTest::StopProfiling() {
  if (profiling_count_ <= 0) {
    return nullptr;
  }
  profiling_count_--;
  printf("StopProfiling: %d\n", profiling_count_);
  if (profiling_count_ == 0) {
    std::unique_ptr<V8CpuProfile> runtime_profile =
        std::make_unique<V8CpuProfile>();
    runtime_profile->start_timestamp = 0;
    runtime_profile->end_timestamp = 1000;
    runtime_profile->samples.push_back(1);
    runtime_profile->samples.push_back(2);
    runtime_profile->time_deltas.push_back(1);
    runtime_profile->time_deltas.push_back(1);
    V8CpuProfileNode node;
    node.id = 1;
    node.call_frame.function_name = "test";
    node.call_frame.script_id = "1";
    node.call_frame.url = "test.js";
    node.call_frame.line_number = 1;
    node.call_frame.column_number = 1;
    runtime_profile->nodes.push_back(std::move(node));
    return runtime_profile;
  }

  return nullptr;
}

TEST(V8RuntimeProfilerTest, V8RuntimeProfilerTotalTest) {
  auto v8_delegate = std::make_shared<V8RuntimeProfilerTest>();
  auto v8_profiler = std::make_shared<V8RuntimeProfiler>(v8_delegate);
  v8_profiler->SetTrackId(1000);
  v8_profiler->SetupProfiling(100);
  v8_profiler->StartProfiling(true);
  v8_profiler->StartProfiling(true);
  ASSERT_EQ(v8_delegate->profiling_count_, static_cast<uint32_t>(2));
  std::unique_ptr<RuntimeProfile> runtime_profile_1 =
      v8_profiler->StopProfiling(true);
  ASSERT_TRUE(runtime_profile_1 == nullptr);
  std::unique_ptr<RuntimeProfile> runtime_profile_2 =
      v8_profiler->StopProfiling(true);
  ASSERT_TRUE(runtime_profile_2 != nullptr);
  ASSERT_EQ(runtime_profile_2->track_id_, static_cast<uint64_t>(1000));
  ASSERT_TRUE(runtime_profile_2->runtime_profile_ != "");
  const std::string& profile = runtime_profile_2->runtime_profile_;
  rapidjson::Document doc;
  bool has_error = doc.Parse(profile.data(), profile.size()).HasParseError() ||
                   !doc.IsObject();
  ASSERT_EQ(has_error, false);
  ASSERT_TRUE(doc.HasMember("profile") && doc["profile"].IsObject());
  const auto& profile_data = doc["profile"].GetObject();
  ASSERT_TRUE(profile_data.HasMember("startTime") &&
              profile_data["startTime"].IsInt64());
  ASSERT_EQ(profile_data["startTime"].GetInt64(), static_cast<int64_t>(0));
  ASSERT_TRUE(profile_data.HasMember("endTime") &&
              profile_data["endTime"].IsInt64());
  ASSERT_EQ(profile_data["endTime"].GetInt64(), static_cast<int64_t>(1000));
  ASSERT_TRUE(profile_data.HasMember("samples") &&
              profile_data["samples"].IsArray());
  ASSERT_EQ(profile_data["samples"].GetArray().Size(), static_cast<size_t>(2));
  ASSERT_TRUE(profile_data.HasMember("timeDeltas") &&
              profile_data["timeDeltas"].IsArray());
  ASSERT_EQ(profile_data["timeDeltas"].GetArray().Size(),
            static_cast<size_t>(2));
  ASSERT_TRUE(profile_data.HasMember("nodes") &&
              profile_data["timeDeltas"].IsArray());
  ASSERT_EQ(profile_data["nodes"].GetArray().Size(), static_cast<size_t>(1));
  const auto& node = (profile_data["nodes"].GetArray())[0];
  ASSERT_TRUE(node.HasMember("id") && node["id"].IsInt() &&
              node["id"].GetInt() == 1);
  ASSERT_TRUE(node.HasMember("callFrame") && node["callFrame"].IsObject());
  const auto& callFrame = node["callFrame"].GetObject();
  ASSERT_TRUE(callFrame.HasMember("functionName") &&
              callFrame["functionName"].IsString());
  const std::string funcName = callFrame["functionName"].GetString();
  ASSERT_TRUE(funcName == "test");
  ASSERT_TRUE(callFrame.HasMember("url") && callFrame["url"].IsString());
  const std::string url = callFrame["url"].GetString();
  ASSERT_TRUE(url == "test.js");
  ASSERT_TRUE(callFrame.HasMember("lineNumber") &&
              callFrame["lineNumber"].IsInt());
  const int lineNumber = callFrame["lineNumber"].GetInt();
  ASSERT_TRUE(lineNumber == 1);
  ASSERT_TRUE(callFrame.HasMember("columnNumber") &&
              callFrame["columnNumber"].IsInt());
  const int columnNumber = callFrame["columnNumber"].GetInt();
  ASSERT_TRUE(columnNumber == 1);
}

}  // namespace testing
}  // namespace profile
}  // namespace lynx
