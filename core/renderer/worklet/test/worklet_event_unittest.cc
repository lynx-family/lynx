// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include <memory>
#include <string>

#include "base/include/fml/message_loop.h"
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
#if ENABLE_TESTBENCH_REPLAY
#include "core/services/replay/testbench_test_replay.h"
#endif  // ENABLE_TESTBENCH_REPLAY
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#if ENABLE_TESTBENCH_REPLAY
#include "third_party/rapidjson/document.h"
#endif  // ENABLE_TESTBENCH_REPLAY

namespace lynx {
namespace base {

#if ENABLE_TESTBENCH_REPLAY
class ScopedTestBenchReplayCapture {
 public:
  ScopedTestBenchReplayCapture()
      : replay_(tasm::replay::TestBenchTestReplay::GetInstance()),
        previous_observer_(replay_.observer_) {
    replay_.SetDevToolObserver(nullptr);
    replay_.StartTest();
  }

  ~ScopedTestBenchReplayCapture() {
    replay_.EndTest("");
    replay_.SetDevToolObserver(previous_observer_);
  }

 private:
  tasm::replay::TestBenchTestReplay& replay_;
  std::shared_ptr<tasm::InspectorCommonObserver> previous_observer_;
};
#endif  // ENABLE_TESTBENCH_REPLAY

class WorkletEventTest : public ::testing::Test {
 protected:
  WorkletEventTest() {}
  ~WorkletEventTest() override {}

  void SetUp() override {
    // Init context
    ctx_->Initialize();

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
    tasm_->EnsureTouchEventHandler();
    touch_event_handler_ = tasm_->touch_event_handler_.get();

    tasm::CompileOptions options;
    options.enable_fiber_arch_ = true;

    // Create default entry and set it to tasm
    auto default_entry = std::make_shared<tasm::TemplateEntry>();
    default_entry->template_bundle_.compile_options_ = options;
    default_entry->SetVm(ctx_);
    default_entry->AttachNapiEnvironment();
    tasm_->template_entries_[tasm::DEFAULT_ENTRY_NAME] = default_entry;

    auto test_entry = std::make_shared<tasm::TemplateEntry>();
    test_entry->template_bundle_.compile_options_ = options;
    test_entry->SetVm(ctx_);
    test_entry->AttachNapiEnvironment();
    tasm_->template_entries_.insert({"test_entry", test_entry});

    // Register Method
    tasm::Renderer::RegisterBuiltin(ctx_.get(), tasm::ArchOption::RADON_ARCH);
    tasm::Renderer::RegisterBuiltin(ctx_.get(), tasm::ArchOption::FIBER_ARCH);
  }

  fml::RefPtr<tasm::Element> CreateElement(const std::string& js_var_name,
                                           int32_t tag) {
    auto quick_context = runtime::MTSRuntime::ToQuickContext(ctx_.get());
    LEPUSValue node_raw_value = quick_context->SearchGlobalData(js_var_name);
    lepus::Value node_value =
        MK_JS_LEPUS_VALUE(quick_context->context(), node_raw_value);
    auto node = reinterpret_cast<tasm::RadonNode*>(node_value.CPoint());
    auto view = manager_->CreateFiberElement("view");
    view->SetAttributeHolder(node->attribute_holder());
    view->CreateElementContainer(false);
    manager_->node_manager()->Record(tag, view.get());
    return view;
  }

  void SendTouchEvent(const std::string& name, int32_t tag) {
    tasm::EventInfo info(tag, 0, 0, 0, 0, 0, 0);
    tasm_->SendTouchEvent(name, info);
  }

  void TearDown() override {}

  std::shared_ptr<runtime::MTSRuntime> ctx_{runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType)};
  runtime::js::NapiRuntimeProxy* napi_proxy_;

  lynx::tasm::TouchEventHandler* touch_event_handler_;
  lynx::tasm::ElementManager* manager_;  // Not Owned
  std::unique_ptr<::testing::NiceMock<tasm::test::MockTasmDelegate>> delegate_;
  std::unique_ptr<runtime::js::NapiEnvironment> napi_environment_;
  std::unique_ptr<tasm::TemplateAssembler> tasm_;
  std::shared_ptr<lynx::tasm::LayoutContext> layout_context_;
};

TEST_F(WorkletEventTest, TestPropagation) {
  auto* comp = new lynx::tasm::RadonComponent(tasm_->page_proxy(), 0, nullptr,
                                              nullptr, nullptr, ctx_.get(), 0);
  auto quick_context = runtime::MTSRuntime::ToQuickContext(ctx_.get());
  quick_context->RegisterGlobalProperty(
      "$comp", LEPUS_MKPTR(LEPUS_TAG_LEPUS_CPOINTER, comp));

  auto page = manager_->CreateFiberElement("page");
  manager_->SetRoot(page.get());
  manager_->SetRootOnLayout(page->impl_id());

  std::string js_source =
      "var counter1 = 0, counter2 = 0, counter3 = 0;"
      "var view1 = _CreateVirtualNode('view', 1);"
      "var view2 = _CreateVirtualNode('view', 2);"
      "var view3 = _CreateVirtualNode('view', 3);"
      "_AppendChild(view1, view2);"
      "_AppendChild(view2, view3);"
      "_SetScriptEventTo(view1, 'bindEvent', 'tap', $comp, (e,lepusComp) => {"
      "counter1++; });"
      "_SetScriptEventTo(view2, 'bindEvent', 'tap', $comp, (e,lepusComp) => { "
      "counter2++; });"
      "_SetScriptEventTo(view3, 'bindEvent', 'tap', $comp, (e,lepusComp) => { "
      "counter3++; });";

  lepus::BytecodeGenerator::GenerateBytecode(ctx_->GetMTSContext(), js_source,
                                             "");
  ctx_->Execute(nullptr);
  tasm_->template_loaded_ = true;

  auto view1 = CreateElement("view1", 1);
  auto view2 = CreateElement("view2", 2);
  auto view3 = CreateElement("view3", 3);
  view1->InsertNode(view2);
  view2->InsertNode(view3);
  page->InsertNode(view1);
  page->FlushProps();

  SendTouchEvent("tap", 1);  // counter1++
  SendTouchEvent("tap", 2);  // counter2++; counter1++
  SendTouchEvent("tap", 3);  // counter3++; counter2++; counter1++

  LEPUSValue counter1 = quick_context->SearchGlobalData("counter1");
  ASSERT_TRUE(LEPUS_IsNumber(counter1));
  EXPECT_EQ(LEPUS_VALUE_GET_INT(counter1), 3);

  LEPUSValue counter2 = quick_context->SearchGlobalData("counter2");
  ASSERT_TRUE(LEPUS_IsNumber(counter2));
  EXPECT_EQ(LEPUS_VALUE_GET_INT(counter2), 2);

  LEPUSValue counter3 = quick_context->SearchGlobalData("counter3");
  ASSERT_TRUE(LEPUS_IsNumber(counter3));
  EXPECT_EQ(LEPUS_VALUE_GET_INT(counter3), 1);
}

TEST_F(WorkletEventTest, TestStopPropagation) {
  auto* comp = new lynx::tasm::RadonComponent(tasm_->page_proxy(), 0, nullptr,
                                              nullptr, nullptr, ctx_.get(), 0);
  auto quick_context = runtime::MTSRuntime::ToQuickContext(ctx_.get());
  quick_context->RegisterGlobalProperty(
      "$comp", LEPUS_MKPTR(LEPUS_TAG_LEPUS_CPOINTER, comp));

  auto page = manager_->CreateFiberElement("page");
  manager_->SetRoot(page.get());
  manager_->SetRootOnLayout(page->impl_id());

  std::string js_source =
      "var counter1 = 0, counter2 = 0, counter3 = 0;"
      "var view1 = _CreateVirtualNode('view', 1);"
      "var view2 = _CreateVirtualNode('view', 2);"
      "var view3 = _CreateVirtualNode('view', 3);"
      "_AppendChild(view1, view2);"
      "_AppendChild(view2, view3);"
      "_SetScriptEventTo(view1, 'bindEvent', 'tap', $comp, (e,lepusComp) => {"
      "counter1++; });"
      "_SetScriptEventTo(view2, 'bindEvent', 'tap', $comp, (e,lepusComp) => { "
      "counter2++; e.stopPropagation(); });"  //  stop propagation
      "_SetScriptEventTo(view3, 'bindEvent', 'tap', $comp, (e,lepusComp) => { "
      "counter3++; });";

  lepus::BytecodeGenerator::GenerateBytecode(quick_context, js_source, "");
  ctx_->Execute(nullptr);
  tasm_->template_loaded_ = true;

  auto view1 = CreateElement("view1", 1);
  auto view2 = CreateElement("view2", 2);
  auto view3 = CreateElement("view3", 3);
  view1->InsertNode(view2);
  view2->InsertNode(view3);
  page->InsertNode(view1);
  page->FlushProps();

  SendTouchEvent("tap", 1);  // counter1++
  SendTouchEvent("tap", 2);  // counter2++;
  SendTouchEvent("tap", 3);  // counter3++; counter2++

  LEPUSValue counter1 = quick_context->SearchGlobalData("counter1");
  ASSERT_TRUE(LEPUS_IsNumber(counter1));
  EXPECT_EQ(LEPUS_VALUE_GET_INT(counter1), 1);

  LEPUSValue counter2 = quick_context->SearchGlobalData("counter2");
  ASSERT_TRUE(LEPUS_IsNumber(counter2));
  EXPECT_EQ(LEPUS_VALUE_GET_INT(counter2), 2);

  LEPUSValue counter3 = quick_context->SearchGlobalData("counter3");
  ASSERT_TRUE(LEPUS_IsNumber(counter3));
  EXPECT_EQ(LEPUS_VALUE_GET_INT(counter3), 1);
}

TEST_F(WorkletEventTest, RefactoredEventKeepsCallJSFunctionCallback) {
  auto config = std::make_shared<tasm::PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableEventHandleRefactor(true);
  tasm_->page_config_ = config;
  manager_->SetConfig(config);

  auto* comp = new tasm::RadonComponent(tasm_->page_proxy(), 0, nullptr,
                                        nullptr, nullptr, ctx_.get(), 0);
  auto* quick_context = runtime::MTSRuntime::ToQuickContext(ctx_.get());
  quick_context->RegisterGlobalProperty(
      "$comp", LEPUS_MKPTR(LEPUS_TAG_LEPUS_CPOINTER, comp));

  auto page = manager_->CreateFiberElement("page");
  manager_->SetRoot(page.get());
  manager_->SetRootOnLayout(page->impl_id());

  std::string js_source =
      "var callback_result = '';"
      "var view = _CreateVirtualNode('view', 1);"
      "_SetScriptEventTo(view, 'bindEvent', 'tap', $comp, (e, lepusComp) => {"
      "lepusComp.callJSFunction('test', {}, (result) => {"
      "callback_result = result; }); });";
  lepus::BytecodeGenerator::GenerateBytecode(quick_context, js_source, "");
  ctx_->Execute(nullptr);
  tasm_->template_loaded_ = true;

  auto node_value = MK_JS_LEPUS_VALUE(quick_context->context(),
                                      quick_context->SearchGlobalData("view"));
  auto* node = static_cast<tasm::RadonNode*>(node_value.CPoint());
  ASSERT_TRUE(node->CreateElementIfNeeded());
  node->DispatchFirstTime();
  auto view = node->GetElementRef();
  ASSERT_NE(view->GetEventListenerMap()->Find("tap"), nullptr);
  view->CreateElementContainer(false);
  manager_->node_manager()->Record(1, view.get());
  page->InsertNode(view);
  page->FlushProps();

#if ENABLE_TESTBENCH_REPLAY
  ScopedTestBenchReplayCapture replay_capture;
#endif  // ENABLE_TESTBENCH_REPLAY
  SendTouchEvent("tap", 1);
#if ENABLE_TESTBENCH_REPLAY
  const auto& replay_records =
      tasm::replay::TestBenchTestReplay::GetInstance().dump_file_;
  ASSERT_EQ(replay_records.count("LepusTouchEvent"), 1u);
  ASSERT_EQ(replay_records.at("LepusTouchEvent").size(), 1u);
  rapidjson::Document replay_event;
  replay_event.Parse(replay_records.at("LepusTouchEvent")[0].c_str());
  ASSERT_TRUE(replay_event.IsObject());
  ASSERT_TRUE(replay_event.HasMember("type"));
  ASSERT_TRUE(replay_event["type"].IsString());
  EXPECT_NE(std::string(replay_event["type"].GetString()).find("tap"),
            std::string::npos);
#endif  // ENABLE_TESTBENCH_REPLAY
  auto& task_handler = tasm_->GetWorkletTaskHandler();
  ASSERT_TRUE(task_handler->HasPendingCalling());
  EXPECT_NE(delegate_->DumpDelegate().find("CallJSFunctionInLepusEvent"),
            std::string::npos);

  tasm_->InvokeLepusComponentCallback(0, tasm::DEFAULT_ENTRY_NAME,
                                      lepus::Value("red"));
  EXPECT_FALSE(task_handler->HasPendingCalling());
  auto callback_result = quick_context->SearchGlobalData("callback_result");
  ASSERT_TRUE(LEPUS_IsString(callback_result));
  auto callback_value =
      MK_JS_LEPUS_VALUE(quick_context->context(), callback_result);
  EXPECT_EQ(callback_value.StdString(), "red");
}

}  // namespace base
}  // namespace lynx
