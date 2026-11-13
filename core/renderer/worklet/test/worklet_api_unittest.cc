// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#define private public
#define protected public

#include <memory>
#include <mutex>

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
#include "core/runtime/common/napi/napi_environment.h"
#include "core/runtime/common/napi/napi_runtime_proxy_quickjs.h"
#include "core/runtime/lepus/bindings/renderer.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/runtime/lepusng/napi/worklet/napi_loader_ui.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/napi/include/napi_module.h"

namespace lynx {
namespace base {
namespace {

int g_module_load_count = 0;
std::once_flag g_module_registration_once;

napi_value InitMTSModuleLoaderTest(napi_env env, napi_value exports) {
  ++g_module_load_count;
  Napi::Object exports_object(env, exports);
  exports_object.Set("message", Napi::String::New(env, "hello from MTS"));
  return exports;
}

napi_module g_mts_module_loader_test = {NAPI_MODULE_VERSION, __FILE__,
                                        InitMTSModuleLoaderTest,
                                        "mts_module_loader_test", nullptr};

}  // namespace

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
    tasm_ = std::make_unique<lynx::tasm::TemplateAssembler>(
        *delegate_.get(), std::move(manager), delegate_.get(), 0);
    ctx_->delegate_ = tasm_.get();

    // Create default entry and set it to tasm
    auto default_entry = std::make_shared<tasm::TemplateEntry>();
    default_entry->SetVm(ctx_);
    ctx_->RegisterLynx(false);
    default_entry->AttachNapiEnvironment();
    tasm_->template_entries_[tasm::DEFAULT_ENTRY_NAME] = default_entry;

    // Register Method
    tasm::Renderer::RegisterBuiltin(ctx_.get(), tasm::ArchOption::RADON_ARCH);
    tasm::Renderer::RegisterBuiltin(ctx_.get(), tasm::ArchOption::FIBER_ARCH);
  }

  void TearDown() override {}

  std::shared_ptr<runtime::MTSRuntime> ctx_{runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType)};
  runtime::js::NapiRuntimeProxy* napi_proxy_;

  lynx::tasm::ElementManager* manager_;  // Not Owned
  std::unique_ptr<::testing::NiceMock<tasm::test::MockTasmDelegate>> delegate_;
  std::unique_ptr<runtime::js::NapiEnvironment> napi_environment_;
  std::unique_ptr<tasm::TemplateAssembler> tasm_;
};

TEST_F(WorkletAPITest, TestLepusLynxTriggerLepusBridge) {
  std::string js_source = R"(
        lepusLynx.triggerLepusBridge("xxx", {}, ()=>{});
    )";

  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  ctx_->Execute(nullptr);

  EXPECT_EQ(delegate_->DumpDelegate(), "TriggerLepusMethodAsync\n");
}

TEST_F(WorkletAPITest, TestLepusLynxSetTimeoutCrash) {
  auto page = manager_->CreateFiberElement("page");
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

  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  ctx_->Execute(nullptr);

  // sleep one second such that when exec loop.RunExpiredTasksNow(), the
  // timeout task can be executed.
  sleep(1);

  loop.RunExpiredTasksNow();
  auto* lynx = static_cast<worklet::NapiLoaderUI*>(
                   tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
                       ->napi_environment()
                       ->delegate())
                   ->lepus_lynx();
  EXPECT_EQ(lynx->task_to_callback_map_.empty(), false);
}

TEST_F(WorkletAPITest, TestLepusLynxTriggerLepusBridgeSync) {
  std::string js_source = R"(
        lepusLynx.triggerLepusBridgeSync("xxx", {});
    )";

  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  ctx_->Execute(nullptr);

  EXPECT_EQ(delegate_->DumpDelegate(), "TriggerLepusMethod\n");
}

TEST_F(WorkletAPITest, TestLynxGetModuleLoader) {
  g_module_load_count = 0;
  std::call_once(g_module_registration_once,
                 []() { napi_module_register_xx(&g_mts_module_loader_test); });

  std::string js_source = R"(
    const module = lynx.getModuleLoader().load("mts_module_loader_test");
    if (module.message !== "hello from MTS") {
      throw new Error("unexpected MTS Node-API module export");
    }
  )";

  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  EXPECT_TRUE(ctx_->Execute(nullptr));
  EXPECT_EQ(g_module_load_count, 1);
}

TEST_F(WorkletAPITest, TestSharedContextUsesOneNapiEnvironment) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto default_entry = tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME);
  auto* environment = default_entry->napi_environment();
  auto env = environment->proxy()->Env();
  auto* loader = static_cast<worklet::NapiLoaderUI*>(environment->delegate());
  auto* lynx = loader->lepus_lynx();
  auto finalized = std::make_shared<bool>(false);
  {
    Napi::HandleScope scope(env);
    lynx->NapiObject().AddFinalizer(
        new std::shared_ptr<bool>(finalized), [](napi_env, void* data, void*) {
          std::unique_ptr<std::shared_ptr<bool>> state(
              static_cast<std::shared_ptr<bool>*>(data));
          **state = true;
        });
  }

  std::string js_source = R"(
    lepusLynx.setTimeout(() => {}, 1000);
  )";
  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  ASSERT_TRUE(ctx_->Execute(nullptr));
  ASSERT_NE(lynx->timer_, nullptr);

  auto component_entry = std::make_shared<tasm::TemplateEntry>();
  component_entry->SetVm(ctx_);
  component_entry->AttachNapiEnvironment();
  tasm_->template_entries_["component_entry"] = component_entry;
  ASSERT_EQ(component_entry->napi_environment(), environment);
  {
    Napi::HandleScope scope(env);
    EXPECT_EQ(env.Global().Get("lepusLynx"), lynx->NapiObject());
  }

  auto* quick_context = runtime::MTSRuntime::ToQuickContext(ctx_.get());
  LEPUS_RunGC(LEPUS_GetRuntime(quick_context->context()));
  ASSERT_FALSE(*finalized);
  tasm_->SetPaused(true);
  EXPECT_TRUE(lynx->paused_);
  EXPECT_TRUE(lynx->timer_->paused_);
  tasm_->SetPaused(false);
  EXPECT_FALSE(lynx->paused_);
  EXPECT_FALSE(lynx->timer_->paused_);

  // TASM tears down bindings before its delegate and rendering state disappear,
  // even when an external reference keeps the runtime and entries alive.
  tasm_.reset();
  EXPECT_EQ(ctx_->GetNapiEnvironment(), nullptr);
  EXPECT_EQ(default_entry->napi_environment(), nullptr);
  EXPECT_EQ(component_entry->napi_environment(), nullptr);
  EXPECT_EQ(quick_context->napi_env(), nullptr);
  EXPECT_EQ(worklet::NapiLoaderUI::GetQuickContextFromNapiEnv(env), nullptr);
  EXPECT_TRUE(*finalized);
}

TEST_F(WorkletAPITest, TestRuntimeDestructionFinalizesNapiEnvironment) {
  auto mts_runtime = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  mts_runtime->Initialize();
  mts_runtime->SetDelegate(tasm_.get());
  mts_runtime->RegisterLynx(false);
  auto entry = std::make_shared<tasm::TemplateEntry>();
  entry->SetVm(mts_runtime);
  entry->AttachNapiEnvironment();
  auto env = entry->napi_environment()->proxy()->Env();
  struct FinalizerState {
    runtime::MTSRuntime* runtime;
    bool finalized = false;
    bool environment_cleared = false;
  };
  auto state = std::make_shared<FinalizerState>();
  state->runtime = mts_runtime.get();
  {
    Napi::HandleScope scope(env);
    env.Global()
        .Get("lepusLynx")
        .As<Napi::Object>()
        .AddFinalizer(
            new std::shared_ptr<FinalizerState>(state),
            [](napi_env, void* data, void*) {
              std::unique_ptr<std::shared_ptr<FinalizerState>> holder(
                  static_cast<std::shared_ptr<FinalizerState>*>(data));
              auto& state = **holder;
              state.environment_cleared =
                  state.runtime->GetNapiEnvironment() == nullptr;
              state.runtime->DetachNapiEnvironment();
              state.finalized = true;
            });
  }

  entry.reset();
  EXPECT_FALSE(state->finalized);
  EXPECT_NE(mts_runtime->GetNapiEnvironment(), nullptr);
  mts_runtime.reset();
  EXPECT_TRUE(state->finalized);
  EXPECT_TRUE(state->environment_cleared);
  EXPECT_EQ(worklet::NapiLoaderUI::GetQuickContextFromNapiEnv(env), nullptr);
  EXPECT_NE(ctx_->GetNapiEnvironment(), nullptr);
}

class WorkletAPIDetachTest : public WorkletAPITest,
                             public ::testing::WithParamInterface<bool> {};

TEST_P(WorkletAPIDetachTest, TestEntryDestructionKeepsModuleLoader) {
  auto default_entry = tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME);
  auto component_entry = std::make_shared<tasm::TemplateEntry>();
  component_entry->SetVm(ctx_);
  component_entry->AttachNapiEnvironment();
  tasm_->template_entries_["component_entry"] = component_entry;
  auto* environment = default_entry->napi_environment();
  auto env = environment->proxy()->Env();
  ASSERT_EQ(component_entry->napi_environment(), environment);
  auto* quick_context = runtime::MTSRuntime::ToQuickContext(ctx_.get());

  std::call_once(g_module_registration_once,
                 []() { napi_module_register_xx(&g_mts_module_loader_test); });
  g_module_load_count = 0;
  std::string js_source = R"(
    if (lynx.getModuleLoader().load("mts_module_loader_test").message !==
        "hello from MTS") {
      throw new Error("unexpected MTS Node-API module export");
    }
  )";
  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  ASSERT_TRUE(ctx_->Execute(nullptr));
  EXPECT_EQ(g_module_load_count, 1);

  // Destroy either entry first. The remaining entry still uses the same
  // runtime, module cache, and bindings.
  if (GetParam()) {
    tasm_->template_entries_.erase("component_entry");
    component_entry.reset();
  } else {
    tasm_->template_entries_.erase(tasm::DEFAULT_ENTRY_NAME);
    default_entry.reset();
  }
  auto remaining_entry = GetParam() ? default_entry : component_entry;
  EXPECT_EQ(remaining_entry->napi_environment(), environment);
  EXPECT_EQ(quick_context->napi_env(), static_cast<napi_env>(env));
  EXPECT_EQ(worklet::NapiLoaderUI::GetQuickContextFromNapiEnv(env),
            quick_context);
  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  EXPECT_TRUE(ctx_->Execute(nullptr));
  EXPECT_EQ(g_module_load_count, 1);

  // Explicit teardown belongs to the whole runtime and remains idempotent.
  remaining_entry->DetachNapiEnvironment();
  remaining_entry->DetachNapiEnvironment();
  EXPECT_EQ(ctx_->GetNapiEnvironment(), nullptr);
  EXPECT_EQ(quick_context->napi_env(), nullptr);
  EXPECT_EQ(worklet::NapiLoaderUI::GetQuickContextFromNapiEnv(env), nullptr);
  remaining_entry->SetPaused(true);
  remaining_entry->SetPaused(false);
  remaining_entry->InvokeLepusBridge(1, lepus::Value());

  js_source = R"(
    if (lynx.getModuleLoader() !== undefined) {
      throw new Error("MTS Node-API module loader is available after detach");
    }
  )";
  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  EXPECT_TRUE(ctx_->Execute(nullptr));
}

INSTANTIATE_TEST_SUITE_P(DetachOrder, WorkletAPIDetachTest, ::testing::Bool());

// Test case for the LepusGesture API
TEST_F(WorkletAPITest, TestLepusGestureAPI) {
  auto element = manager_->CreateFiberElement("view");

  // Create a LepusGesture instance associated with the RadonElement
  auto lepus_gesture = std::unique_ptr<worklet::LepusGesture>(
      worklet::LepusGesture::Create(element->impl_id(), tasm_.get()));

  auto lepus_context =
      runtime::MTSRuntime::ToQuickContext(ctx_.get())->context();
  // Create and convert a Lepus value (integer 1) to a Napi value
  auto js_1 = LEPUS_NewInt32(lepus_context, 1);
  lepus::Value value_1 = MK_JS_LEPUS_VALUE(lepus_context, js_1);
  auto napi_value_1 = worklet::ValueConverter::ConvertLepusValueToNapiValue(
      tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
          ->napi_environment()
          ->proxy()
          ->Env(),
      value_1);

  // Create and convert another Lepus value (integer 2) to a Napi value
  auto js_2 = LEPUS_NewInt32(lepus_context, 2);
  lepus::Value value_2 = MK_JS_LEPUS_VALUE(lepus_context, js_2);
  auto napi_value_2 = worklet::ValueConverter::ConvertLepusValueToNapiValue(
      tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
          ->napi_environment()
          ->proxy()
          ->Env(),
      value_2);

  // Create and convert yet another Lepus value (integer 3) to a Napi value
  auto js_3 = LEPUS_NewInt32(lepus_context, 3);
  lepus::Value value_3 = MK_JS_LEPUS_VALUE(lepus_context, js_3);
  auto napi_value_3 = worklet::ValueConverter::ConvertLepusValueToNapiValue(
      tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
          ->napi_environment()
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
  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  ctx_->Execute(nullptr);

  auto* lynx = static_cast<worklet::NapiLoaderUI*>(
                   tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
                       ->napi_environment()
                       ->delegate())
                   ->lepus_lynx();
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
  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  ctx_->Execute(nullptr);

  auto* lynx = static_cast<worklet::NapiLoaderUI*>(
                   tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
                       ->napi_environment()
                       ->delegate())
                   ->lepus_lynx();
  EXPECT_EQ(lynx->task_to_callback_map_.empty(), true);
}

TEST_F(WorkletAPITest, TestLepusLynxSetInterval) {
  std::string js_source = R"(
        lepusLynx.setInterval(()=>{}, 1000);
  )";

  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();
  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  ctx_->Execute(nullptr);

  auto* lynx = static_cast<worklet::NapiLoaderUI*>(
                   tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
                       ->napi_environment()
                       ->delegate())
                   ->lepus_lynx();
  EXPECT_EQ(lynx->task_to_callback_map_.empty(), false);
}

TEST_F(WorkletAPITest, TestLepusLynxClearInterval) {
  std::string js_source = R"(
        let id = lepusLynx.setInterval(()=>{}, 1000);
        lepusLynx.clearInterval(id);
  )";

  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();
  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  ctx_->Execute(nullptr);

  auto* lynx = static_cast<worklet::NapiLoaderUI*>(
                   tasm_->FindEntry(tasm::DEFAULT_ENTRY_NAME)
                       ->napi_environment()
                       ->delegate())
                   ->lepus_lynx();
  EXPECT_EQ(lynx->task_to_callback_map_.empty(), true);
}

}  // namespace base
}  // namespace lynx
