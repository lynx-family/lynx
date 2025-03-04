// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/profile/runtime_profiler.h"

#include "base/trace/native/trace_controller.h"
#include "core/runtime/profile/runtime_profiler_manager.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace profile {
namespace testing {

class RuntimeProfilerManagerTest : public RuntimeProfilerManager {
 public:
  size_t GetRuntimeProfilerSize();

 private:
  void SaveRuntimeProfile(
      const std::shared_ptr<RuntimeProfile>& runtime_profile,
      const int32_t index);
};

size_t RuntimeProfilerManagerTest::GetRuntimeProfilerSize() {
  return runtime_profilers_.size();
}

void RuntimeProfilerManagerTest::SaveRuntimeProfile(
    const std::shared_ptr<RuntimeProfile>& runtime_profile,
    const int32_t index) {
  ASSERT_EQ(runtime_profile->runtime_profile_, "profile");
}

class RuntimeProfilerTest : public RuntimeProfiler {
 public:
  RuntimeProfilerTest() = default;
  ~RuntimeProfilerTest() = default;
  virtual void StartProfiling(bool is_create) override;
  virtual std::unique_ptr<RuntimeProfile> StopProfiling(
      bool is_destory) override;
  virtual void SetupProfiling(const int32_t sampling_interval) override;
  virtual trace::RuntimeProfilerType GetType() override;
};

void RuntimeProfilerTest::StartProfiling(bool is_create) {
  auto task = [] { printf("StartProfiling\n"); };

  RuntimeProfiler::StartProfiling(std::move(task), is_create);
}

void RuntimeProfilerTest::SetupProfiling(const int32_t sampling_interval) {
  ASSERT_EQ(sampling_interval, static_cast<int32_t>(100));
  auto task = [sampling_interval] {
    printf("SetupProfiling: %d\n", sampling_interval);
  };

  RuntimeProfiler::SetupProfiling(std::move(task));
}

std::unique_ptr<RuntimeProfile> RuntimeProfilerTest::StopProfiling(
    bool is_destory) {
  std::string runtime_profile = "";
  auto task = [&runtime_profile] { runtime_profile = "profile"; };
  RuntimeProfiler::StopProfiling(std::move(task), is_destory);
  return std::make_unique<RuntimeProfile>(runtime_profile, 1000);
}

trace::RuntimeProfilerType RuntimeProfilerTest::GetType() {
  return trace::RuntimeProfilerType::quickjs;
}

TEST(RuntimeProfilerTest, RuntimeProfilerSyncTotalTest) {
  RuntimeProfilerTest profiler_;
  profiler_.SetupProfiling(100);
  profiler_.StartProfiling(true);
  auto runtime_profile = profiler_.StopProfiling(true);
  ASSERT_TRUE(runtime_profile->track_id_ == static_cast<uint64_t>(1000));
  ASSERT_EQ(runtime_profile->runtime_profile_, "profile");
}

TEST(RuntimeProfilerManagerTest, RuntimeProfilerManagerTotalTest) {
  auto manager = std::make_shared<RuntimeProfilerManagerTest>();
  auto profiler = std::make_shared<RuntimeProfilerTest>();
  manager->AddRuntimeProfiler(profiler);
  manager->AddRuntimeProfiler(std::make_shared<RuntimeProfilerTest>());
  ASSERT_EQ(manager->GetRuntimeProfilerSize(), static_cast<size_t>(2));
  manager->RemoveRuntimeProfiler(profiler);
  ASSERT_EQ(manager->GetRuntimeProfilerSize(), static_cast<size_t>(1));
  auto trace_config = std::make_shared<trace::TraceConfig>();
  trace_config->js_profile_interval = 100;
  manager->DispatchSetup(trace_config);
  manager->DispatchBegin();
  manager->DispatchEnd();
  SUCCEED();
}

}  // namespace testing
}  // namespace profile
}  // namespace lynx
