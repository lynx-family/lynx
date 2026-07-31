// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include "base/include/fml/message_loop.h"
#define private public
#include "core/renderer/utils/lynx_env.h"
#undef private

#include "core/renderer/tasm/testing/event_tracker_mock.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace {

thread_local std::string g_coverage_dump_for_testing;
thread_local int32_t g_coverage_runtime_id_for_testing = -1;
thread_local int g_coverage_dump_call_count_for_testing = 0;

}  // namespace

extern "C" {

// Host unit-test PrimJS may lag behind the target PrimJS library that provides
// the coverage APIs. These weak fakes exercise Lynx's coverage control flow
// without adding a compatibility implementation to production. A real VM
// implementation, when linked, takes precedence over these definitions.
__attribute__((weak)) LEPUSValue LEPUS_Eval_WITH_COVERAGE(
    LEPUSContext* ctx, const char* input, size_t input_len,
    const char* filename, int eval_flags, int start_line_number,
    int32_t runtime_id) {
  g_coverage_runtime_id_for_testing = runtime_id;
  g_coverage_dump_for_testing =
      std::string("{\"url\":\"") + filename + "\",\"ranges\":[0,1,1]}";
  return LEPUS_Eval2(ctx, input, input_len, filename, eval_flags,
                     start_line_number);
}

__attribute__((weak)) const char* JS_GetCoverageDumpString(LEPUSContext*,
                                                           int32_t runtime_id,
                                                           size_t* length) {
  ++g_coverage_dump_call_count_for_testing;
  if (runtime_id != g_coverage_runtime_id_for_testing ||
      g_coverage_dump_for_testing.empty()) {
    return nullptr;
  }
  *length = g_coverage_dump_for_testing.size();
  auto* dump = static_cast<char*>(std::malloc(*length + 1));
  if (!dump) {
    *length = 0;
    return nullptr;
  }
  std::memcpy(dump, g_coverage_dump_for_testing.c_str(), *length + 1);
  return dump;
}

__attribute__((weak)) void JS_FreeCoverageDumpString(const char* dump) {
  std::free(const_cast<char*>(dump));
}

}  // extern "C"

namespace lynx {
namespace runtime {
namespace js {
namespace test {

namespace {

class ScopedJSCoverageSamplingEnv {
 public:
  explicit ScopedJSCoverageSamplingEnv(std::string value)
      : env_(tasm::LynxEnv::GetInstance()) {
    std::lock_guard<std::recursive_mutex> lock(env_.external_env_mutex_);
    const auto it = env_.external_env_map_.find(
        tasm::LynxEnv::Key::JS_COVERAGE_PAGE_SAMPLING_BASIS_POINTS);
    if (it != env_.external_env_map_.end()) {
      previous_value_ = it->second;
    }
    env_.external_env_map_
        [tasm::LynxEnv::Key::JS_COVERAGE_PAGE_SAMPLING_BASIS_POINTS] =
        std::move(value);
  }

  ~ScopedJSCoverageSamplingEnv() {
    std::lock_guard<std::recursive_mutex> lock(env_.external_env_mutex_);
    if (previous_value_) {
      env_.external_env_map_
          [tasm::LynxEnv::Key::JS_COVERAGE_PAGE_SAMPLING_BASIS_POINTS] =
          *previous_value_;
    } else {
      env_.external_env_map_.erase(
          tasm::LynxEnv::Key::JS_COVERAGE_PAGE_SAMPLING_BASIS_POINTS);
    }
  }

 private:
  tasm::LynxEnv& env_;
  std::optional<std::string> previous_value_;
};

class CoverageRuntimeDelegate : public JSRuntimeDelegate {
 public:
  std::shared_ptr<Buffer> GetBytecode(const std::string&) override {
    ++get_bytecode_call_count;
    return std::make_shared<StringBuffer>("unexpected cached bytecode");
  }

  void OnJSIException(const JSIException&) override {}

  int get_bytecode_call_count{0};
};

}  // namespace

TEST(QuickjsRuntimeCoverageTest, SourceBypassesCacheAndProducesCoverageDump) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  ScopedJSCoverageSamplingEnv sampling_env(
      std::to_string(tasm::LynxEnv::kJSCoverageSamplingBasisPointsMax));
  auto runtime_delegate = std::make_shared<CoverageRuntimeDelegate>();
  auto runtime = std::make_unique<QuickjsRuntime>();
  auto vm = runtime->createVM(nullptr);
  auto context = runtime->createContext(vm);

  constexpr int32_t kRuntimeId = 42;
  JSRuntimeExternalParams external_params;
  external_params.runtime_id = kRuntimeId;
  external_params.enable_user_bytecode = true;
  external_params.bytecode_source_url = "coverage-template.js";
  external_params.delegate = runtime_delegate;
  runtime->SetExternalParams(std::move(external_params));
  runtime->InitRuntime(context);

  constexpr char kSourceUrl[] = "coverage-source.js";
  auto preparation = runtime->prepareJavaScript(
      std::make_shared<StringBuffer>(
          "globalThis.coverageResult = 'instrumented';"),
      kSourceUrl);
  ASSERT_NE(preparation, nullptr);
  const auto* quickjs_preparation =
      static_cast<const QuickjsJavaScriptPreparation*>(preparation.get());
  EXPECT_EQ(quickjs_preparation->Bytecode(), nullptr);
  EXPECT_NE(quickjs_preparation->Source(), nullptr);
  EXPECT_EQ(runtime_delegate->get_bytecode_call_count, 0);

  const auto result = runtime->evaluatePreparedJavaScript(*preparation);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(runtime->global()
                .getProperty(*runtime, "coverageResult")
                ->getString(*runtime)
                .utf8(*runtime),
            "instrumented");

  size_t dump_length = 0;
  const char* coverage_dump = JS_GetCoverageDumpString(
      runtime->getJSContext(), kRuntimeId, &dump_length);
  ASSERT_NE(coverage_dump, nullptr);
  EXPECT_GT(dump_length, 0u);
  EXPECT_NE(std::string(coverage_dump, dump_length).find(kSourceUrl),
            std::string::npos);
  JS_FreeCoverageDumpString(coverage_dump);
}

TEST(QuickjsRuntimeCoverageTest, CoverageIdIsCreatedOnFirstDumpAndStaysStable) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  ScopedJSCoverageSamplingEnv sampling_env(
      std::to_string(tasm::LynxEnv::kJSCoverageSamplingBasisPointsMax));
  auto runtime = std::make_unique<QuickjsRuntime>();

  auto vm = runtime->createVM(nullptr);
  auto context = runtime->createContext(vm);
  constexpr int32_t kRuntimeId = 43;
  JSRuntimeExternalParams external_params;
  external_params.runtime_id = kRuntimeId;
  runtime->SetExternalParams(std::move(external_params));
  runtime->InitRuntime(context);

  const auto result = runtime->evaluateJavaScript(
      std::make_shared<StringBuffer>("globalThis.coverageIdTest = true;"),
      "coverage-id.js");
  ASSERT_TRUE(result.has_value());

  // Exercise repeated dumps through the public lifecycle hook so the test
  // validates reported behavior without exposing QuickjsRuntime internals.
  auto report_event = tasm::report::EventTrackerWaitableEvent::Await();
  report_event->Reset();
  runtime->BeforeDestroy();
  report_event->Wait();
  ASSERT_EQ(tasm::report::EventTrackerWaitableEvent::stack_.size(), 1u);
  const auto first_dump_props =
      tasm::report::EventTrackerWaitableEvent::stack_.front().GetStringProps();
  const auto first_coverage_id = first_dump_props.find("coverage_id");
  ASSERT_NE(first_coverage_id, first_dump_props.end());
  EXPECT_FALSE(first_coverage_id->second.empty());
  EXPECT_EQ(first_coverage_id->second.find(std::to_string(kRuntimeId) + "_"),
            0u);

  report_event->Reset();
  runtime->BeforeDestroy();
  report_event->Wait();
  ASSERT_EQ(tasm::report::EventTrackerWaitableEvent::stack_.size(), 1u);
  const auto second_dump_props =
      tasm::report::EventTrackerWaitableEvent::stack_.front().GetStringProps();
  const auto second_coverage_id = second_dump_props.find("coverage_id");
  ASSERT_NE(second_coverage_id, second_dump_props.end());
  EXPECT_EQ(second_coverage_id->second, first_coverage_id->second);
}

TEST(QuickjsRuntimeCoverageTest, CoverageDisabledRuntimeDoesNotDumpCoverage) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  ScopedJSCoverageSamplingEnv sampling_env("0");
  auto runtime = std::make_unique<QuickjsRuntime>();

  auto vm = runtime->createVM(nullptr);
  auto context = runtime->createContext(vm);
  runtime->InitRuntime(context);

  g_coverage_dump_call_count_for_testing = 0;
  runtime->BeforeDestroy();
  EXPECT_EQ(g_coverage_dump_call_count_for_testing, 0);
}

}  // namespace test
}  // namespace js
}  // namespace runtime
}  // namespace lynx
