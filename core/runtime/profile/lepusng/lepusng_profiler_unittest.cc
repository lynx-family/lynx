// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/profile/lepusng/lepusng_profiler.h"

#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace profile {
namespace test {

class ContextDelegateTest : public runtime::MTSRuntime::Delegate {
 public:
  virtual const std::string& TargetSdkVersion() override {
    static const std::string sdk_version = "3.1";
    return sdk_version;
  };
  virtual void ReportError(base::LynxError error){};
  virtual void OnBTSConsoleEvent(const std::string& func_name,
                                 const std::string& args){};
  virtual void ReportGCTimingEvent(const char* start, const char* end){};
  virtual void OnRuntimeGC(
      std::unordered_map<std::string, std::string> mem_info) override{};
  virtual fml::RefPtr<fml::TaskRunner> GetLepusTimedTaskRunner() {
    return nullptr;
  };
  virtual void OnScriptingStart() override{};
  virtual void OnScriptingEnd() override{};
};

TEST(LepusNGProfilerTest, LepusNGProfilerTotalTest) {
  auto context = lynx::runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  context->Initialize();
  void* assembler = new ContextDelegateTest();
  lepus::QuickContext* quick_ctx =
      runtime::MTSRuntime::ToQuickContext(context.get());
  LEPUSValue self = LEPUS_MKPTR(LEPUS_TAG_LEPUS_CPOINTER, assembler);
  quick_ctx->RegisterGlobalProperty("$kTemplateAssembler", self);
  auto lepusng_profiler = std::make_shared<LepusNGProfiler>(context);
  quick_ctx->SetRuntimeProfiler(lepusng_profiler);
  std::string codeStr = "var b = 1;";
  lepus::BytecodeGenerator::GenerateBytecode(context->GetMTSContext(), codeStr,
                                             "3.1");
  lepusng_profiler->SetupProfiling(100);
  lepusng_profiler->StartProfiling(true);
  context->Execute(nullptr);
  auto runtime_profile = lepusng_profiler->StopProfiling(true);
  ASSERT_NE(runtime_profile, nullptr);
  ASSERT_NE(runtime_profile->runtime_profile_, "");
  quick_ctx->RemoveRuntimeProfiler();
}

}  // namespace test
}  // namespace profile
}  // namespace runtime
}  // namespace lynx
