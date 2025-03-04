// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#define private public
#define protected public

#include <memory>

#include "base/include/fml/message_loop.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/worklet/base/worklet_utils.h"
#include "core/renderer/worklet/lepus_component.h"
#include "core/renderer/worklet/lepus_element.h"
#include "core/renderer/worklet/lepus_gesture.h"
#include "core/renderer/worklet/lepus_lynx.h"
#include "core/runtime/bindings/lepus/renderer.h"
#include "core/runtime/bindings/napi/napi_environment.h"
#include "core/runtime/bindings/napi/napi_runtime_proxy_quickjs.h"
#include "core/runtime/bindings/napi/worklet/napi_loader_ui.h"
#include "core/runtime/vm/lepus/bytecode_generator.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

class WorkletAPITest : public ::testing::Test {
 protected:
  WorkletAPITest() {}
  ~WorkletAPITest() override {}

  void SetUp() override {
    // Init context
    ctx_->Initialize();
    base::UIThread::Init();
    // Create Delegate
    static constexpr int32_t kWidth = 1024;
    static constexpr int32_t kHeight = 768;
    static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
    static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;
    auto lynx_env_config =
        tasm::LynxEnvConfig(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                            kDefaultPhysicalPixelsPerLayoutUnit);

    delegate_ =
        std::make_unique<::testing::NiceMock<tasm::test::MockTasmDelegate>>();

    // Create Element Manager
    auto manager = std::make_unique<tasm::ElementManager>(
        std::make_unique<tasm::MockPaintingContext>(), delegate_.get(),
        lynx_env_config);

    manager_ = manager.get();

    // Init tasm
    tasm_ = std::make_shared<lynx::tasm::TemplateAssembler>(
        *delegate_.get(), std::move(manager), 0);
    ctx_->delegate_ = tasm_.get();

    // Create default entry and set it to tasm
    auto default_entry = std::make_shared<tasm::TemplateEntry>();
    default_entry->SetVm(ctx_);
    default_entry->AttachNapiEnvironment();
    tasm_->template_entries_[tasm::DEFAULT_ENTRY_NAME] = default_entry;

    // Register Method
    tasm::Utils::RegisterNGBuiltin(ctx_.get());
    tasm::Renderer::RegisterNGBuiltin(ctx_.get(), tasm::ArchOption::RADON_ARCH);
    tasm::Renderer::RegisterNGBuiltin(ctx_.get(), tasm::ArchOption::FIBER_ARCH);
  }

  void TearDown() override {}

  std::shared_ptr<lepus::QuickContext> ctx_{new lepus::QuickContext()};
  piper::NapiRuntimeProxy* napi_proxy_;

  lynx::tasm::ElementManager* manager_;  // Not Owned
  std::unique_ptr<::testing::NiceMock<tasm::test::MockTasmDelegate>> delegate_;
  std::unique_ptr<piper::NapiEnvironment> napi_environment_;
  std::shared_ptr<tasm::TemplateAssembler> tasm_;
};

TEST_F(WorkletAPITest, TestLepusLynxTriggerLepusBridge) {
  std::string js_source = R"(
        lepusLynx.triggerLepusBridge("xxx", {}, ()=>{});
    )";

  lepus::BytecodeGenerator::GenerateBytecode(ctx_.get(), js_source, "");
  ctx_->Execute();

  EXPECT_EQ(delegate_->DumpDelegate(), "TriggerLepusMethodAsync\n");
}

TEST_F(WorkletAPITest, TestLepusLynxSetTimeoutCrash) {
  auto page = manager_->CreateNode("page", nullptr);
  manager_->SetRoot(page.get());
  manager_->SetRootOnLayout(page->impl_id());
  page->FlushProps();

  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto& loop = fml::MessageLoop::GetCurrent();
  std::string js_source = R"(
            lepusLynx.setTimeout(()=>{
                lepusLynx.clearTimeout(0);
            }, 1);
      )";

  lepus::BytecodeGenerator::GenerateBytecode(ctx_.get(), js_source, "");
  ctx_->Execute();

  // sleep one second such that when exec loop.RunExpiredTasksNow(), the
  // timeout task can be executed.
  sleep(1);

  loop.RunExpiredTasksNow();
  auto* lynx = static_cast<worklet::NapiLoaderUI*>(
                   tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
                       ->napi_environment_->delegate())
                   ->lynx_;
  EXPECT_EQ(lynx->task_to_callback_map_.empty(), false);
}

TEST_F(WorkletAPITest, TestLepusLynxTriggerLepusBridgeSync) {
  std::string js_source = R"(
        lepusLynx.triggerLepusBridgeSync("xxx", {});
    )";

  lepus::BytecodeGenerator::GenerateBytecode(ctx_.get(), js_source, "");
  ctx_->Execute();

  EXPECT_EQ(delegate_->DumpDelegate(), "TriggerLepusMethod\n");
}

// Test case for the LepusGesture API
TEST_F(WorkletAPITest, TestLepusGestureAPI) {
  // Create a RadonElement using the manager
  auto element = manager_->CreateNode("view", nullptr);

  // Create a LepusGesture instance associated with the RadonElement
  auto lepus_gesture =
      std::unique_ptr<worklet::LepusGesture>(worklet::LepusGesture::Create(
          element->impl_id(), tasm_->shared_from_this()));

  // Create and convert a Lepus value (integer 1) to a Napi value
  auto js_1 = LEPUS_NewInt32(ctx_->context(), 1);
  lepus::Value value_1(ctx_->context(), js_1);
  auto napi_value_1 = worklet::ValueConverter::ConvertLepusValueToNapiValue(
      tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
          ->napi_environment_.get()
          ->proxy()
          ->Env(),
      value_1);

  // Create and convert another Lepus value (integer 2) to a Napi value
  auto js_2 = LEPUS_NewInt32(ctx_->context(), 2);
  lepus::Value value_2(ctx_->context(), js_2);
  auto napi_value_2 = worklet::ValueConverter::ConvertLepusValueToNapiValue(
      tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
          ->napi_environment_.get()
          ->proxy()
          ->Env(),
      value_2);

  // Create and convert yet another Lepus value (integer 3) to a Napi value
  auto js_3 = LEPUS_NewInt32(ctx_->context(), 3);
  lepus::Value value_3(ctx_->context(), js_3);
  auto napi_value_3 = worklet::ValueConverter::ConvertLepusValueToNapiValue(
      tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
          ->napi_environment_.get()
          ->proxy()
          ->Env(),
      value_3);

  // Trigger different LepusGesture actions using the Napi values
  lepus_gesture->Active(napi_value_1.ToNumber());
  lepus_gesture->Fail(napi_value_2.ToNumber());
  lepus_gesture->End(napi_value_3.ToNumber());

  // Record the RadonElement in the node manager
  manager_->node_manager()->Record(element->impl_id(), element.get());

  // Trigger the same LepusGesture actions again
  lepus_gesture->Active(napi_value_1.ToNumber());
  lepus_gesture->Fail(napi_value_2.ToNumber());
  lepus_gesture->End(napi_value_3.ToNumber());
}

TEST_F(WorkletAPITest, TestLepusLynxSetTimeout) {
  std::string js_source = R"(
        lepusLynx.setTimeout(()=>{}, 1000);
  )";

  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();
  lepus::BytecodeGenerator::GenerateBytecode(ctx_.get(), js_source, "");
  ctx_->Execute();

  auto* lynx = static_cast<worklet::NapiLoaderUI*>(
                   tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
                       ->napi_environment_->delegate())
                   ->lynx_;
  EXPECT_EQ(lynx->task_to_callback_map_.empty(), false);
}

TEST_F(WorkletAPITest, TestCallJSFunctionInLepusEvent) {
  auto dict = lepus::Dictionary::Create();
  dict.get()->SetValue(base::String("1"), lepus::Value(1));
  dict.get()->SetValue(base::String("2"), lepus::Value(2));
  dict.get()->SetValue(base::String("3"), lepus::Value(3));

  tasm_->CallJSFunctionInLepusEvent("1", "test", lepus::Value(dict));
  EXPECT_EQ(delegate_->DumpDelegate(), "");

  tasm_->template_loaded_ = true;
  tasm_->CallJSFunctionInLepusEvent("1", "test", lepus::Value(dict));
  EXPECT_EQ(delegate_->DumpDelegate(),
            "CallJSFunctionInLepusEvent 1 test {\"1\":1,\"2\":2,\"3\":3}\n");
}

TEST_F(WorkletAPITest, TestLepusLynxClearTimeout) {
  std::string js_source = R"(
        let id = lepusLynx.setTimeout(()=>{}, 1000);
        lepusLynx.clearTimeout(id);
  )";

  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();
  lepus::BytecodeGenerator::GenerateBytecode(ctx_.get(), js_source, "");
  ctx_->Execute();

  auto* lynx = static_cast<worklet::NapiLoaderUI*>(
                   tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
                       ->napi_environment_->delegate())
                   ->lynx_;
  EXPECT_EQ(lynx->task_to_callback_map_.empty(), true);
}

TEST_F(WorkletAPITest, TestLepusLynxSetInterval) {
  std::string js_source = R"(
        lepusLynx.setInterval(()=>{}, 1000);
  )";

  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();
  lepus::BytecodeGenerator::GenerateBytecode(ctx_.get(), js_source, "");
  ctx_->Execute();

  auto* lynx = static_cast<worklet::NapiLoaderUI*>(
                   tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
                       ->napi_environment_->delegate())
                   ->lynx_;
  EXPECT_EQ(lynx->task_to_callback_map_.empty(), false);
}

TEST_F(WorkletAPITest, TestLepusLynxClearInterval) {
  std::string js_source = R"(
        let id = lepusLynx.setInterval(()=>{}, 1000);
        lepusLynx.clearInterval(id);
  )";

  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();
  lepus::BytecodeGenerator::GenerateBytecode(ctx_.get(), js_source, "");
  ctx_->Execute();

  auto* lynx = static_cast<worklet::NapiLoaderUI*>(
                   tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
                       ->napi_environment_->delegate())
                   ->lynx_;
  EXPECT_EQ(lynx->task_to_callback_map_.empty(), true);
}

}  // namespace base
}  // namespace lynx
