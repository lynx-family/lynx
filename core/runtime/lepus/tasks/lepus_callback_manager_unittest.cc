// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public

#include "core/runtime/lepus/tasks/lepus_callback_manager.h"

#include <vector>

#include "base/include/value/base_value.h"
#include "core/runtime/lepus/context.h"
#include "core/runtime/lepus/mts_context.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace air {
namespace testing {

class MockMTSContext final : public lepus::MTSContext {
 public:
  MockMTSContext() : lepus::MTSContext(nullptr, nullptr) {}
  ~MockMTSContext() override = default;

  void Initialize() override {}
  lepus::ContextType Type() const override {
    return lepus::ContextType::LepusNGContextType;
  }

  void RegisterGlobalFunction(const lepus::RenderBindingFunction* /*funcs*/,
                              size_t /*size*/) override {}
  void RegisterObjectFunction(lepus::Value& /*obj*/,
                              const lepus::RenderBindingFunction* /*funcs*/,
                              size_t /*size*/) override {}

  bool UpdateTopLevelVariableByPath(base::Vector<std::string>& /*path*/,
                                    const lepus::Value& /*val*/) override {
    return true;
  }

  bool CheckTableShadowUpdatedWithTopLevelVariable(
      const lepus::Value& /*update*/) override {
    return false;
  }

  void SetGlobalData(const base::String& /*name*/,
                     lepus::Value /*value*/) override {}
  void ResetGlobalData(const base::String& /*name*/,
                       lepus::Value /*value*/) override {}
  lepus::Value GetGlobalData(const base::String& /*name*/) override {
    return lepus::Value();
  }

  bool DeSerialize(const lepus::ContextBundle& /*bundle*/, bool /*needExecute*/,
                   lepus::Value* /*ret*/,
                   const char* /*file_name*/ = nullptr) override {
    return false;
  }

  void ApplyConfig(const std::shared_ptr<tasm::PageConfig>& /*config*/,
                   const tasm::CompileOptions& /*options*/) override {}

  lepus::Value ReportFatalError(const std::string& /*error_message*/,
                                bool /*exit*/, int32_t /*code*/) override {
    return lepus::Value();
  }

  void BindCurrentThread() override {}

  lepus::Value CallArgs(const base::String& /*name*/,
                        const lepus::Value* /*args*/[], size_t /*args_count*/,
                        bool /*pause_suppression_mode*/) override {
    return lepus::Value();
  }

  lepus::Value CallClosureArgs(const lepus::Value& /*closure*/,
                               const lepus::Value* /*args*/[],
                               size_t /*args_count*/) override {
    return lepus::Value(1);
  }

  bool ExecuteBinaryWithBundle(const lepus::ContextBundle* /*bundle*/,
                               lepus::Value* /*ret_val*/) override {
    return true;
  }
};

class MockLepusContext final : public lepus::Context {
 public:
  MockLepusContext()
      : lepus::Context(lepus::ContextType::LepusNGContextType,
                       /*disable_tracing_gc=*/false,
                       /*runtime_mode=*/0, tasm::PageOptions()) {
    delegate_ = &delegate_impl_;
    mts_context_ = std::make_unique<MockMTSContext>();
  }

 private:
  class DummyDelegate final : public lepus::Context::Delegate {
   public:
    const std::string& TargetSdkVersion() override { return sdk_version_; }
    void ReportError(base::LynxError /*error*/) override {}
    void OnBTSConsoleEvent(const std::string& /*func_name*/,
                           const std::string& /*args*/) override {}
    void ReportGCTimingEvent(const char* /*start*/,
                             const char* /*end*/) override {}
    void OnRuntimeGC(
        std::unordered_map<std::string, std::string> /*mem_info*/) override {}
    fml::RefPtr<fml::TaskRunner> GetLepusTimedTaskRunner() override {
      return nullptr;
    }
    void OnScriptingStart() override {}
    void OnScriptingEnd() override {}

   private:
    std::string sdk_version_{"test"};
  };

  DummyDelegate delegate_impl_;
};

class LepusCallbackManagerTest : public ::testing::Test {
 public:
  LepusCallbackManagerTest() {}
  ~LepusCallbackManagerTest() override {}

  std::unique_ptr<tasm::LepusCallbackManager> lepus_callback_manager;
  MockLepusContext context;

  void SetUp() override {
    lepus_callback_manager = std::make_unique<tasm::LepusCallbackManager>();
  }
};

TEST_F(LepusCallbackManagerTest, CacheTask) {
  int64_t id_1 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task1"));
  int64_t id_2 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task2"));
  int64_t id_3 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task3"));
  ASSERT_TRUE(id_1 == 1);
  ASSERT_TRUE(id_2 == 2);
  ASSERT_TRUE(id_3 == 3);
}

TEST_F(LepusCallbackManagerTest, InvokeTask) {
  int64_t id_1 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task1"));
  EXPECT_TRUE(id_1 == 1);
  std::vector<lepus::Value> args;
  args.emplace_back(lepus::Value(1));
  lepus_callback_manager->InvokeTask(id_1, args);
  EXPECT_TRUE(lepus_callback_manager->task_map_.empty());
  int64_t id_2 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task2"));
  EXPECT_TRUE(id_2 == 2);
  EXPECT_TRUE(!lepus_callback_manager->task_map_.empty());
}

}  // namespace testing
}  // namespace air
}  // namespace lynx
