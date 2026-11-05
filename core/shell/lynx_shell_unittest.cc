// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// FIXME(heshan):will delete after refact manufactor
// use for hook
#define private public
#define protected public

#include "core/shell/lynx_shell.h"

#include <thread>
#include <vector>

#include "base/include/debug/lynx_error.h"
#include "base/include/value/base_value.h"
#include "core/base/threading/vsync_monitor.h"
#include "core/inspector/observer/native_module_record_observer.h"
#include "core/public/pub_value.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/renderer/tasm/testing/event_tracker_mock.h"
#include "core/renderer/ui_wrapper/painting/empty/painting_context_implementation.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/js_executor.h"
#include "core/runtime/js/mock_template_delegate.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "core/services/performance/memory_monitor/memory_record.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/shell/lynx_shell_builder.h"
#include "core/shell/testing/mock_native_facade.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

namespace {

class FrameRateTestPlatformInvoker : public TasmPlatformInvoker {
 public:
  void OnPageConfigDecoded(
      const std::shared_ptr<tasm::PageConfig>& config) override {}
  void OnRunPipelineFinished() override {}
  lepus::Value TriggerLepusMethod(const std::string& method_name,
                                  const lepus::Value& args) override {
    return lepus::Value();
  }
  void TriggerLepusMethodAsync(const std::string& method_name,
                               const lepus::Value& args) override {}
  std::string TranslateResourceForTheme(const std::string& res_id,
                                        const std::string& theme_key) override {
    return {};
  }
  void GetI18nResource(const std::string& channel,
                       const std::string& fallback_url) override {}
};

class FrameRateTestVSyncMonitor : public base::VSyncMonitor {
 public:
  void SetHighRefreshRate() override { ++high_refresh_rate_requests; }

  int high_refresh_rate_requests{0};
};

}  // namespace

TEST(TasmMediatorTest, PreferredFpsHighWithoutBackgroundRuntime) {
  TasmMediator mediator(nullptr, nullptr, nullptr,
                        std::make_unique<FrameRateTestPlatformInvoker>(),
                        nullptr);
  auto monitor = std::make_shared<FrameRateTestVSyncMonitor>();
  mediator.vsync_monitor_ = monitor;
  auto config = std::make_shared<tasm::PageConfig>();
  config->SetPreferredFps("high");

  mediator.OnPageConfigDecoded(config);

  EXPECT_EQ(monitor->high_refresh_rate_requests, 1);
}

TEST(TasmMediatorTest, PreferredFpsAutoDoesNotRequestHighRefreshRate) {
  TasmMediator mediator(nullptr, nullptr, nullptr,
                        std::make_unique<FrameRateTestPlatformInvoker>(),
                        nullptr);
  auto monitor = std::make_shared<FrameRateTestVSyncMonitor>();
  mediator.vsync_monitor_ = monitor;

  mediator.OnPageConfigDecoded(std::make_shared<tasm::PageConfig>());

  EXPECT_EQ(monitor->high_refresh_rate_requests, 0);
}

TEST(TasmMediatorTest, PreferredFpsHighKeepsMTSMonitorLazy) {
  TasmMediator mediator(nullptr, nullptr, nullptr,
                        std::make_unique<FrameRateTestPlatformInvoker>(),
                        nullptr);
  auto config = std::make_shared<tasm::PageConfig>();
  config->SetPreferredFps("high");

  mediator.OnPageConfigDecoded(config);

  EXPECT_EQ(mediator.vsync_monitor_, nullptr);
}

class LynxShellTest : public ::testing::Test {
 protected:
  LynxShellTest() = default;
  ~LynxShellTest() override = default;

  void SetUp() override {
    base::UIThread::Init();
    auto& env = lynx::tasm::LynxEnv::GetInstance();
    env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDebugEnabled, false);
    env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDevToolEnable, false);
    facade_ = new MockNativeFacade;

    // FIXME(heshan): tricky, here must ensure manufactor not create thread
    // POSIX thread exit may cause crash...
    lynx::shell::ShellOption option;
    option.view_id_ = 18;
    shell_.reset(
        lynx::shell::LynxShellBuilder()
            .SetNativeFacade(std::unique_ptr<NativeFacade>(facade_))
            .SetLayoutContextPlatformImpl(nullptr)
            .SetPaintingContextPlatformImpl(
                std::make_unique<tasm::PaintingContextPlatformImpl>())
            .SetStrategy(base::ThreadStrategyForRendering::ALL_ON_UI)
            .SetShellOption(option)
            .SetPropBundleCreator(
                std::make_shared<lynx::tasm::PropBundleCreatorDefault>())
            .build());

    // Finish engine initialization on the original runner before replacing it.
    shell_->engine_actor_->ActSync([](auto&) {});

    // hook runners, use for test
    shell_->runners_ =
        MockRunnerManufactor(base::ThreadStrategyForRendering::MULTI_THREADS);
    shell_->facade_actor_->runner_ = shell_->runners_.GetUITaskRunner();
    shell_->engine_actor_->runner_ = shell_->runners_.GetTASMTaskRunner();
    shell_->layout_actor_->runner_ = shell_->runners_.GetLayoutTaskRunner();
    shell_->runtime_actor_ = std::make_shared<LynxActor<BTSRuntime>>(
        nullptr, shell_->runners_.GetJSTaskRunner());
    tasm_mediator_ = shell_->tasm_mediator_;
    arwe_ = facade_->arwe;
  }

  void TearDown() override {
    shell_->Destroy();
    arwe_->Wait();
    // check destroy life cycle

    ASSERT_TRUE(shell_->facade_actor_->impl_ == nullptr);

    // check call after destroy, no crash is ok
    shell_->OnEnterForeground();
    shell_->OnEnterBackground();

    shell_ = nullptr;
    auto& env = lynx::tasm::LynxEnv::GetInstance();
    env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDebugEnabled, false);
    env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDevToolEnable, false);
  }

  MockNativeFacade* facade_;
  std::unique_ptr<LynxShell> shell_;
  TasmMediator* tasm_mediator_ = nullptr;
  std::shared_ptr<fml::AutoResetWaitableEvent> arwe_;
};

TEST_F(LynxShellTest, LifecycleControlsElementVsyncOnEngineThread) {
  auto is_paused = [this]() {
    return shell_->engine_actor_->ActSync([](auto& engine) {
      return engine->GetTasm()
          ->page_proxy()
          ->element_manager()
          ->IsElementVsyncPaused();
    });
  };
  EXPECT_FALSE(is_paused());
  shell_->OnEnterBackground();
  EXPECT_TRUE(is_paused());
  shell_->OnEnterForeground();
  EXPECT_FALSE(is_paused());
}

TEST_F(LynxShellTest, ElementVsyncUsesUnifiedPauseState) {
  auto is_paused = [this]() {
    return shell_->engine_actor_->ActSync([](auto& engine) {
      auto* manager = engine->GetTasm()->page_proxy()->element_manager().get();
      return manager->IsElementVsyncPaused();
    });
  };
  shell_->OnEnterBackground();
  EXPECT_TRUE(is_paused());
  shell_->OnEnterForeground();
  EXPECT_FALSE(is_paused());

  shell_->Pause();
  EXPECT_TRUE(is_paused());

  // Lifecycle and explicit pause both update the same VSync state.
  shell_->OnEnterBackground();
  shell_->Resume();
  EXPECT_FALSE(is_paused());

  // The foreground lifecycle update resumes the same state.
  shell_->Pause();
  shell_->OnEnterForeground();
  EXPECT_FALSE(is_paused());
}

TEST_F(LynxShellTest, ResetLoadOrderOnlyForEmbeddedMode) {
  shell_->ui_operation_queue_->is_engine_async_ = true;
  shell_->ui_operation_queue_->CreateImpl();
  ASSERT_EQ(shell_->ui_operation_queue_->UpdateNativeUpdateDataOrder(), 1u);
  ASSERT_EQ(shell_->ui_operation_queue_->UpdateNativeUpdateDataOrder(), 2u);

  auto pipeline_options = std::make_shared<tasm::PipelineOptions>();
  pipeline_options->native_update_data_order_ = 42;
  shell_->ResetNativeUpdateDataOrderForLoad(pipeline_options);
  EXPECT_EQ(shell_->ui_operation_queue_->GetNativeUpdateDataOrder(), 2u);
  EXPECT_EQ(pipeline_options->native_update_data_order_, 42u);

  tasm::PageOptions page_options;
  page_options.SetEmbeddedMode(tasm::EmbeddedMode::EMBEDDED_MODE_BASE);
  shell_->page_options_ = page_options;
  shell_->ResetNativeUpdateDataOrderForLoad(pipeline_options);
  EXPECT_EQ(shell_->ui_operation_queue_->GetNativeUpdateDataOrder(), 0u);
  EXPECT_EQ(pipeline_options->native_update_data_order_, 0u);
  EXPECT_EQ(shell_->ui_operation_queue_->UpdateNativeUpdateDataOrder(), 1u);
}

// for async operation, use arwe wait for result
TEST_F(LynxShellTest, OnTemplateLoaded) {
  std::string url = "url";
  tasm_mediator_->OnTemplateLoaded(url);
  arwe_->Wait();
  ASSERT_TRUE(*facade_);
  ASSERT_EQ(std::any_cast<std::string>((*facade_)["url"]), url);
}

TEST_F(LynxShellTest, OnSSRHydrateFinished) {
  std::string url = "url";
  tasm_mediator_->OnSSRHydrateFinished(url);
  arwe_->Wait();
  ASSERT_TRUE(*facade_);
  ASSERT_EQ(std::any_cast<std::string>((*facade_)["url"]), url);
}

TEST_F(LynxShellTest, OnErrorOccurred) {
  int32_t error_code = 777;
  std::string msg = "shell test error msg";
  tasm_mediator_->OnErrorOccurred(
      base::LynxError(error_code, msg, "", base::LynxErrorLevel::Error));
  arwe_->Wait();
  ASSERT_TRUE(*facade_);
  ASSERT_EQ(std::any_cast<int32_t>((*facade_)["error_code"]), error_code);
  ASSERT_EQ(std::any_cast<std::string>((*facade_)["msg"]), msg);
}

TEST_F(LynxShellTest, OnConfigUpdated) {
  lepus::Value data = lepus::Value("test data");
  tasm_mediator_->OnConfigUpdated(data);
  arwe_->Wait();
  ASSERT_TRUE(*facade_);
  ASSERT_EQ(std::any_cast<lepus::Value>((*facade_)["data"]), data);
}

TEST_F(LynxShellTest, SendGlobalEventToMTSWithoutBackgroundRuntime) {
  shell_->runtime_actor_.reset();
  int event_count = 0;
  lepus::Value received_args;
  shell_->engine_actor_->ActSync([&](auto& engine) {
    auto* proxy = engine->GetTasm()->GetContextProxy(
        runtime::ContextProxy::Type::kNative);
    proxy->AddEventListener(
        "custom-event",
        std::make_shared<event::ClosureEventListener>([&](lepus::Value args) {
          EXPECT_TRUE(MockRunnerManufactor::InOnTASMThread());
          ++event_count;
          received_args = lepus::Value::ShallowCopy(args);
        }));
  });

  auto params = lepus::CArray::Create();
  params->emplace_back("first");
  auto detail = lepus::Dictionary::Create();
  detail->SetValue("value", lepus::Value(42));
  params->emplace_back(std::move(detail));
  shell_->SendGlobalEventToMTS("custom-event", lepus::Value(params));
  shell_->engine_actor_->ActSync([](auto&) {});

  EXPECT_EQ(event_count, 1);
  ASSERT_TRUE(received_args.IsArray());
  ASSERT_EQ(received_args.GetLength(), 2U);
  EXPECT_EQ(received_args.GetProperty(0), lepus::Value("custom-event"));
  auto received_params = received_args.GetProperty(1);
  ASSERT_TRUE(received_params.IsArray());
  ASSERT_EQ(received_params.GetLength(), 2U);
  EXPECT_EQ(received_params.GetProperty(0), lepus::Value("first"));
  EXPECT_EQ(received_params.GetProperty(1).GetProperty("value"),
            lepus::Value(42));
}

TEST_F(LynxShellTest, SendGlobalEventToMTSPreservesEmptyArguments) {
  shell_->runtime_actor_.reset();
  int event_count = 0;
  shell_->engine_actor_->ActSync([&](auto& engine) {
    auto* proxy = engine->GetTasm()->GetContextProxy(
        runtime::ContextProxy::Type::kNative);
    proxy->AddEventListener(
        "empty-event",
        std::make_shared<event::ClosureEventListener>([&](lepus::Value args) {
          ++event_count;
          EXPECT_EQ(args.GetProperty(0), lepus::Value("empty-event"));
          auto params = args.GetProperty(1);
          ASSERT_TRUE(params.IsArray());
          EXPECT_EQ(params.GetLength(), 0U);
        }));
  });

  shell_->SendGlobalEventToMTS("empty-event",
                               lepus::Value(lepus::CArray::Create()));
  shell_->engine_actor_->ActSync([](auto&) {});
  EXPECT_EQ(event_count, 1);
}

TEST_F(LynxShellTest, GlobalEventToMTSRequiresMatchingNamedListener) {
  shell_->runtime_actor_.reset();
  tasm_mediator_->runtime_actor_.reset();

  for (bool from_template_assembler : {false, true}) {
    SCOPED_TRACE(from_template_assembler);
    int dispatched_events = 0;
    int matching_events = 0;
    int unrelated_events = 0;
    lepus::Value received_args;
    auto info = lepus::Value(lepus::Dictionary::Create());
    info.SetProperty("value", lepus::Value(42));
    auto params = lepus::CArray::Create();
    params->push_back(info);

    shell_->engine_actor_->ActSync([&](auto& engine) {
      auto* tasm = engine->GetTasm();
      tasm->SetEnableBTSRuntime(true);
      EXPECT_FALSE(tasm->ShouldSendEventToMainThread());
      auto* proxy = tasm->GetContextProxy(runtime::ContextProxy::Type::kNative);
      proxy->SetListenerBeforePublishEvent(
          std::make_unique<event::ClosureEventListener>(
              [&](lepus::Value) { ++dispatched_events; }));
      for (const auto& name :
           {"other-event", runtime::kMessageEventTypeGlobalEvent}) {
        proxy->AddEventListener(name,
                                std::make_shared<event::ClosureEventListener>(
                                    [&](lepus::Value) { ++unrelated_events; }));
      }
    });

    auto send_event = [&]() {
      if (from_template_assembler) {
        shell_->engine_actor_->Act([&](auto& engine) {
          engine->GetTasm()->SendGlobalEvent("named-event", info);
        });
      } else {
        shell_->SendGlobalEventToMTS("named-event", lepus::Value(params));
      }
      shell_->engine_actor_->ActSync([](auto&) {});
    };

    send_event();
    EXPECT_EQ(dispatched_events, 0);
    EXPECT_EQ(unrelated_events, 0);

    shell_->engine_actor_->ActSync([&](auto& engine) {
      auto* proxy = engine->GetTasm()->GetContextProxy(
          runtime::ContextProxy::Type::kNative);
      proxy->AddEventListener(
          "named-event",
          std::make_shared<event::ClosureEventListener>([&](lepus::Value args) {
            EXPECT_TRUE(MockRunnerManufactor::InOnTASMThread());
            ++matching_events;
            received_args = lepus::Value::ShallowCopy(args);
          }));
    });

    send_event();
    EXPECT_EQ(dispatched_events, 1);
    EXPECT_EQ(matching_events, 1);
    EXPECT_EQ(unrelated_events, 0);
    ASSERT_TRUE(received_args.IsArray());
    ASSERT_EQ(received_args.GetLength(), 2U);
    EXPECT_EQ(received_args.GetProperty(0), lepus::Value("named-event"));
    EXPECT_EQ(received_args.GetProperty(1), lepus::Value(params));

    shell_->engine_actor_->ActSync([](auto& engine) {
      engine->GetTasm()
          ->GetContextProxy(runtime::ContextProxy::Type::kNative)
          ->RemoveEventListeners("named-event");
    });
    send_event();
    EXPECT_EQ(dispatched_events, 1);
    EXPECT_EQ(matching_events, 1);
    EXPECT_EQ(unrelated_events, 0);

    shell_->engine_actor_->ActSync([](auto& engine) {
      auto* proxy = engine->GetTasm()->GetContextProxy(
          runtime::ContextProxy::Type::kNative);
      proxy->SetListenerBeforePublishEvent(nullptr);
      proxy->RemoveEventListeners("other-event");
      proxy->RemoveEventListeners(runtime::kMessageEventTypeGlobalEvent);
    });
  }
}

TEST_F(LynxShellTest, BTSGlobalEventKeepsJSDeliveryWithoutForwardingToMTS) {
  class RecordingDelegate : public runtime::test::MockTemplateDelegate {
   public:
    event::DispatchEventResult DispatchMessageEvent(
        fml::RefPtr<runtime::MessageEvent> event) override {
      if (event->GetOriginType() == runtime::ContextProxy::Type::kNative &&
          event->GetTargetType() == runtime::ContextProxy::Type::kCoreContext) {
        ++forwarded_global_events;
      }
      return {event::EventCancelType::kNotCanceled, false};
    }
    int forwarded_global_events = 0;
  };
  class RecordingObserver : public runtime::js::NativeModuleRecordObserver {
   public:
    void OnRecord(const lepus::Value&) override {}
    std::shared_ptr<runtime::js::NativeModuleInvocationContext>
    CreateInvocation(const std::string&, const std::string&) override {
      return nullptr;
    }
    void OnGlobalEvent(const std::string& name,
                       const lepus::Value& arguments) override {
      names.push_back(name);
      params.push_back(arguments);
    }
    std::vector<std::string> names;
    std::vector<lepus::Value> params;
  };

  auto delegate = std::make_unique<RecordingDelegate>();
  auto* delegate_ptr = delegate.get();
  BTSRuntime bts_runtime("-1", {}, 18, std::move(delegate), "",
                         LynxRuntimeFlags::PENDING_CORE_JS_LOAD |
                             LynxRuntimeFlags::FORCE_USE_LIGHT_WEIGHT_JS_ENGINE,
                         tasm::PageOptions(18));
  bts_runtime.Init(std::make_shared<pub::LynxNativeModuleManager>(), nullptr,
                   {});
  bts_runtime.state_ = BTSRuntime::State::kRuntimeReady;
  auto* js_runtime = bts_runtime.GetJSRuntimeWeak().Lock();
  ASSERT_NE(js_runtime, nullptr);
  runtime::js::Scope scope(*js_runtime);
  auto js_app = js_runtime->evaluateJavaScript(
      std::make_shared<runtime::js::StringBuffer>(
          "({callFunction: function(module, method, args) {"
          "  globalThis.receivedGlobalEvents.push([module, method, args]);"
          "}})"),
      "global-event-test.js");
  ASSERT_TRUE(js_app.has_value());
  js_runtime->global().setProperty(
      *js_runtime, "receivedGlobalEvents",
      *runtime::js::Array::createWithLength(*js_runtime, 0));
  bts_runtime.app_->SetJsAppObj(js_app->getObject(*js_runtime));
  auto observer = std::make_shared<RecordingObserver>();
  bts_runtime.app_->SetNativeModuleRecordObserver(observer);

  auto listener_params = lepus::CArray::Create();
  listener_params->emplace_back("payload");
  listener_params->emplace_back(42);
  for (const auto& params : {lepus::Value(lepus::CArray::Create()),
                             lepus::Value(std::move(listener_params))}) {
    auto args = lepus::CArray::Create();
    args->emplace_back("native-event");
    args->push_back(params);
    bts_runtime.CallJSFunction("GlobalEventEmitter", "emit",
                               lepus::Value(std::move(args)));
  }

  auto received = bts_runtime.app_->ParseJSValueToLepusValue(
      *js_runtime->global().getProperty(*js_runtime, "receivedGlobalEvents"),
      PAGE_GROUP_ID);
  ASSERT_TRUE(received.has_value());
  ASSERT_EQ(received->GetLength(), 2U);
  ASSERT_EQ(observer->names.size(), 2U);
  for (size_t i = 0; i < 2; ++i) {
    auto call = received->GetProperty(i);
    EXPECT_EQ(call.GetProperty(0), lepus::Value("GlobalEventEmitter"));
    EXPECT_EQ(call.GetProperty(1), lepus::Value("emit"));
    EXPECT_EQ(call.GetProperty(2).GetProperty(0), lepus::Value("native-event"));
    EXPECT_EQ(observer->names[i], "native-event");
    EXPECT_EQ(observer->params[i], call.GetProperty(2).GetProperty(1));
  }
  EXPECT_EQ(delegate_ptr->forwarded_global_events, 0);
}

TEST_F(LynxShellTest, OnShouldSendEventToMainThreadChanged) {
  tasm_mediator_->OnShouldSendEventToMainThreadChanged(false);
  arwe_->Wait();
  ASSERT_TRUE(*facade_);
  ASSERT_FALSE(
      std::any_cast<bool>((*facade_)["should_send_event_to_main_thread"]));
}

TEST_F(LynxShellTest, LoadTemplateResetsShouldSendEventToMainThread) {
  auto set_should_send_event_to_main_thread = [this](bool enable) {
    tasm_mediator_->OnShouldSendEventToMainThreadChanged(enable);
    arwe_->Wait();
    ASSERT_EQ(
        std::any_cast<bool>((*facade_)["should_send_event_to_main_thread"]),
        enable);
  };
  auto pipeline_options = std::make_shared<tasm::PipelineOptions>();
  shell_->engine_actor_->enable_ = false;

  set_should_send_event_to_main_thread(false);
  shell_->LoadTemplate("url", {}, pipeline_options, nullptr);
  arwe_->Wait();
  EXPECT_TRUE(
      std::any_cast<bool>((*facade_)["should_send_event_to_main_thread"]));

  set_should_send_event_to_main_thread(false);
  shell_->LoadTemplateBundle("url", tasm::LynxTemplateBundle(),
                             pipeline_options, nullptr);
  arwe_->Wait();
  EXPECT_TRUE(
      std::any_cast<bool>((*facade_)["should_send_event_to_main_thread"]));

  shell_->engine_actor_->enable_ = true;
}

TEST_F(LynxShellTest, OnUpdateDataWithoutChange) {
  tasm_mediator_->OnUpdateDataWithoutChange();
  arwe_->Wait();
  ASSERT_TRUE(*facade_);
}

TEST_F(LynxShellTest, EmbeddedTimingForwardsLoadBundleEndToPlatform) {
  tasm::PageOptions page_options;
  page_options.SetEmbeddedMode(tasm::EmbeddedMode::EMBEDDED_MODE_BASE);
  tasm_mediator_->SetPageOptions(page_options);

  constexpr uint64_t kTimestampUs = 123456;
  const std::string pipeline_id = "pipeline-id";
  tasm::Timing timing(pipeline_id);
  timing.timings_.insert_if_absent(tasm::timing::kLoadBundleEnd, kTimestampUs);

  tasm_mediator_->SetTiming(std::move(timing));
  arwe_->Wait();

  EXPECT_TRUE(*facade_);
  EXPECT_EQ(std::any_cast<std::string>((*facade_)["timing_key"]),
            tasm::timing::kLoadBundleEnd);
  EXPECT_EQ(std::any_cast<uint64_t>((*facade_)["timestamp_us"]), kTimestampUs);
  EXPECT_EQ(std::any_cast<std::string>((*facade_)["pipeline_id"]), pipeline_id);
}

TEST_F(LynxShellTest, InitRuntimeUpdatesBTSGroupIdGenericInfo) {
  constexpr const char* kGroupId = "test_group";

  // Previous fixture destruction can enqueue element statistic events on the
  // shared mock TASM runner. Drain the producer and report queues before
  // resetting the shared event tracker state.
  shell_->engine_actor_->GetRunner()->PostSyncTask([]() {});
  tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostSyncTask(
      []() {});

  auto event = tasm::report::EventTrackerWaitableEvent::Await();
  event->Reset();
  tasm::report::EventTrackerWaitableEvent::instance_id_ =
      tasm::report::kUnknownInstanceId;
  tasm::report::EventTrackerWaitableEvent::generic_info_by_instance_.erase(
      shell_->GetInstanceId());

  shell_->enable_runtime_ = false;
  shell_->InitRuntime(
      kGroupId, nullptr, nullptr,
      [](const std::shared_ptr<LynxActor<BTSRuntime>>&) {},
      std::vector<std::string>(), 0, "");
  event->Wait();

  EXPECT_EQ(tasm::report::EventTrackerWaitableEvent::instance_id_,
            shell_->GetInstanceId());
  const auto& generic_info =
      tasm::report::EventTrackerWaitableEvent::generic_info_by_instance_.at(
          shell_->GetInstanceId());
  auto bts_group_id = generic_info.find(tasm::report::kPropBTSGroupId);
  ASSERT_NE(bts_group_id, generic_info.end());
  EXPECT_EQ(bts_group_id->second, kGroupId);
}

TEST_F(LynxShellTest, GetAllPerformanceEntries) {
  auto& env = lynx::tasm::LynxEnv::GetInstance();
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDebugEnabled, true);
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDevToolEnable, true);

  shell_->perf_controller_actor_->ActSync([](auto& controller) {
    auto entry = controller->GetValueFactory()->CreateMap();
    entry->PushStringToMap("entryType", "pipeline");
    entry->PushStringToMap("name", "testMetric");
    entry->PushDoubleToMap("startTime", 1.5);
    controller->OnPerformanceEvent(std::move(entry));
  });

  lepus::Value entries = shell_->GetAllPerformanceEntries();
  ASSERT_TRUE(entries.IsArray());
  ASSERT_EQ(entries.Array()->size(), 1u);

  lepus::Value first_entry = entries.Array()->get(0);
  ASSERT_TRUE(first_entry.IsTable());
  ASSERT_EQ(first_entry.Table()->GetValue("entryType").String().str(),
            "pipeline");
  ASSERT_EQ(first_entry.Table()->GetValue("name").String().str(), "testMetric");
  ASSERT_DOUBLE_EQ(first_entry.Table()->GetValue("startTime").Number(), 1.5);
  ASSERT_EQ(first_entry.Table()->GetValue("instanceId").Int32(),
            shell_->perf_controller_actor_->GetInstanceId());
}

TEST_F(LynxShellTest, GetAllPerformanceEntriesReturnsEmptyWhenDevToolDisabled) {
  auto& env = lynx::tasm::LynxEnv::GetInstance();
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDebugEnabled, false);
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDevToolEnable, false);

  shell_->perf_controller_actor_->ActSync([](auto& controller) {
    auto entry = controller->GetValueFactory()->CreateMap();
    entry->PushStringToMap("entryType", "pipeline");
    entry->PushStringToMap("name", "testMetric");
    entry->PushDoubleToMap("startTime", 1.5);
    controller->OnPerformanceEvent(std::move(entry));
  });

  lepus::Value entries = shell_->GetAllPerformanceEntries();
  ASSERT_TRUE(entries.IsArray());
  ASSERT_EQ(entries.Array()->size(), 0u);
}

TEST_F(LynxShellTest, ResetTimingBeforeReloadClearsPerformanceEntries) {
  auto& env = lynx::tasm::LynxEnv::GetInstance();
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDebugEnabled, true);
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDevToolEnable, true);

  shell_->perf_controller_actor_->ActSync([](auto& controller) {
    auto entry = controller->GetValueFactory()->CreateMap();
    entry->PushStringToMap("entryType", "metric");
    entry->PushStringToMap("name", "beforeReload");
    controller->OnPerformanceEvent(std::move(entry));
  });

  shell_->ResetTimingBeforeReload();
  shell_->perf_controller_actor_->ActSync([](auto& controller) {});

  lepus::Value entries = shell_->GetAllPerformanceEntries();
  ASSERT_TRUE(entries.IsArray());
  ASSERT_EQ(entries.Array()->size(), 0u);
}

}  // namespace shell
}  // namespace lynx
