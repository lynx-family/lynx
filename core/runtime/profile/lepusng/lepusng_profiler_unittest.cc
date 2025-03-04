// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/profile/lepusng/lepusng_profiler.h"

#include "core/runtime/vm/lepus/bytecode_generator.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace profile {
namespace test {

class ContextDelegateTest : public lepus::Context::Delegate {
 public:
  virtual const std::string& TargetSdkVersion() override {
    static const std::string sdk_version = "3.1";
    return sdk_version;
  };
  virtual void ReportError(base::LynxError error){};
  virtual void PrintMsgToJS(const std::string& level, const std::string& msg){};
  virtual void ReportGCTimingEvent(const char* start, const char* end){};
  virtual fml::RefPtr<fml::TaskRunner> GetLepusTimedTaskRunner() {
    return nullptr;
  };
};

TEST(LepusNGProfilerTest, LepusNGProfilerTotalTest) {
  auto context = lynx::lepus::Context::CreateContext(true);
  context->Initialize();
  void* assembler = new ContextDelegateTest();
  lepus::QuickContext* quick_ctx = lepus::QuickContext::Cast(context.get());
  LEPUSValue self = LEPUS_MKPTR(LEPUS_TAG_LEPUS_CPOINTER, assembler);
  quick_ctx->RegisterGlobalProperty("$kTemplateAssembler", self);
  auto lepusng_profiler = std::make_shared<LepusNGProfiler>(context);
  quick_ctx->SetRuntimeProfiler(lepusng_profiler);
  std::string codeStr = "var b = 1;";
  lepus::BytecodeGenerator::GenerateBytecode(context.get(), codeStr, "3.1");
  lepusng_profiler->SetupProfiling(100);
  lepusng_profiler->StartProfiling(true);
  quick_ctx->Execute(nullptr);
  auto runtime_profile = lepusng_profiler->StopProfiling(true);
  ASSERT_NE(runtime_profile, nullptr);
  ASSERT_NE(runtime_profile->runtime_profile_, "");
  quick_ctx->RemoveRuntimeProfiler();
}

}  // namespace test
}  // namespace profile
}  // namespace lynx
