// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/ui_wrapper/painting/empty/painting_context_implementation.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/shell/lynx_entity_id_generator.h"
#include "core/shell/lynx_shell.h"
#include "core/shell/lynx_shell_builder.h"
#include "core/shell/testing/mock_native_facade.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

class MockLazyBundleLoader : public LazyBundleLoader {
 public:
  MockLazyBundleLoader() = default;
  ~MockLazyBundleLoader() = default;
  void RequireTemplate(RadonLazyComponent* lazy_bundle, const std::string& url,
                       int instance_id) override {}

};  // MockLazyBundleLoader
}  // namespace tasm
namespace shell {

namespace {

bool LogContextEquals(const base::LogContext& lhs,
                      const base::LogContext& rhs) {
  return lhs.view_id == rhs.view_id && lhs.engine_id == rhs.engine_id &&
         lhs.runtime_id == rhs.runtime_id;
}

bool EngineTreeLogContextEquals(LynxEngine* engine,
                                const base::LogContext& context) {
  auto* tasm = engine->GetTasm();
  auto* element_manager = tasm->page_proxy()->element_manager().get();
  return LogContextEquals(engine->GetLogContext(), context) &&
         LogContextEquals(tasm->GetLogContext(), context) &&
         LogContextEquals(element_manager->GetLogContext(), context) &&
         LogContextEquals(element_manager->catalyzer()->GetLogContext(),
                          context) &&
         LogContextEquals(element_manager->painting_context()->GetLogContext(),
                          context);
}

}  // namespace

class LynxShellBuilderTest : public ::testing::Test {
 protected:
  LynxShellBuilderTest() = default;
  ~LynxShellBuilderTest() override = default;

  void SetUp() override {
    lynx_env_config_ =
        std::make_unique<lynx::tasm::LynxEnvConfig>(60, 90, 1.f, 1.f);
    loader_ = std::make_shared<lynx::tasm::MockLazyBundleLoader>();

    option_ = std::make_unique<lynx::shell::ShellOption>();
    option_->view_id_ = 17;

    shell_builder_ = std::make_unique<LynxShellBuilder>();
  }

  void TearDown() override { shell_ = nullptr; }

  std::unique_ptr<LynxShell> BuildShellForEngineHandoff(
      base::LynxEntityId view_id, LynxEngineWrapper* engine_wrapper,
      MockNativeFacade** facade_out) {
    ShellOption option;
    option.view_id_ = view_id;
    option.enable_js_ = false;
    auto facade = std::make_unique<MockNativeFacade>();
    *facade_out = facade.get();
    auto painting_context_creator = [](LynxShell*) {
      return std::make_unique<tasm::PaintingContextPlatformImpl>();
    };
    return std::unique_ptr<LynxShell>(
        LynxShellBuilder()
            .SetNativeFacade(std::move(facade))
            .SetPaintingContextCreator(painting_context_creator)
            .SetLynxEnvConfig(*lynx_env_config_)
            .SetLazyBundleLoader(loader_)
            .SetEnableLayoutOnly(enable_layout_only_)
            .SetLayoutContextPlatformImpl(nullptr)
            .SetStrategy(strategy_)
            .SetShellOption(option)
            .SetLynxEngineWrapper(engine_wrapper)
            .build());
  }

  intptr_t facade_;
  intptr_t painting_context_;
  std::unique_ptr<lynx::tasm::LynxEnvConfig> lynx_env_config_ = nullptr;
  std::shared_ptr<lynx::tasm::MockLazyBundleLoader> loader_ = nullptr;
  bool enable_pre_update_data_ = true;
  bool enable_layout_only_ = false;
  std::string locale_ = "LynxShellBuilderTotalTest";
  lynx::base::ThreadStrategyForRendering strategy_ =
      lynx::base::ThreadStrategyForRendering::ALL_ON_UI;
  std::unique_ptr<lynx::shell::ShellOption> option_ = nullptr;

  std::unique_ptr<LynxShell> shell_;
  std::unique_ptr<LynxShellBuilder> shell_builder_ = nullptr;

};  // LynxShellBuilderTest

TEST_F(LynxShellBuilderTest, LynxShellBuilderTotalTest) {
  auto facade = std::make_unique<MockNativeFacade>();
  facade_ = reinterpret_cast<intptr_t>(facade.get());

  auto painting_context =
      std::make_unique<lynx::tasm::PaintingContextPlatformImpl>();
  painting_context_ = reinterpret_cast<intptr_t>(painting_context.get());
  auto painting_context_creator = [&](lynx::shell::LynxShell* shell) {
    return std::move(painting_context);
  };

  shell_.reset((*shell_builder_)
                   .SetNativeFacade(std::move(facade))
                   .SetPaintingContextCreator(painting_context_creator)
                   .SetLynxEnvConfig(*lynx_env_config_)
                   .SetEnableElementManagerVsyncMonitor(true)
                   .SetLazyBundleLoader(loader_)
                   .SetEnablePreUpdateData(enable_pre_update_data_)
                   .SetEnableLayoutOnly(enable_layout_only_)
                   .SetTasmLocale(locale_)
                   .SetLayoutContextPlatformImpl(nullptr)
                   .SetStrategy(strategy_)
                   .SetShellOption(*option_)
                   .build());

  shell_->runtime_actor_ = std::make_shared<LynxActor<BTSRuntime>>(
      nullptr, shell_->runners_.GetUITaskRunner());

  const auto shell_context = shell_->GetLogContextSnapshot();
  EXPECT_EQ(shell_context.view_id, option_->view_id_);
  EXPECT_NE(shell_context.engine_id, base::kUnavailableLynxEntityId);
  EXPECT_EQ(shell_context.runtime_id, base::kUnavailableLynxEntityId);
  EXPECT_EQ(shell_->engine_actor_->Impl()->GetLogContext().engine_id,
            shell_context.engine_id);
  EXPECT_EQ(shell_->layout_actor_->Impl()->GetLogContext().engine_id,
            shell_context.engine_id);
  EXPECT_NE(&shell_->engine_actor_->Impl()->GetLogContext(),
            &shell_->engine_actor_->Impl()->GetTasm()->GetLogContext());
  EXPECT_TRUE(
      EngineTreeLogContextEquals(shell_->engine_actor_->Impl(), shell_context));
  const auto& performance_context =
      shell_->perf_controller_actor_->Impl()->GetLogContext();
  EXPECT_EQ(performance_context.view_id, shell_context.view_id);
  EXPECT_EQ(performance_context.engine_id, base::kUnavailableLynxEntityId);
  EXPECT_EQ(performance_context.runtime_id, base::kUnavailableLynxEntityId);
  auto* performance = shell_->perf_controller_actor_->Impl();
  EXPECT_NE(&performance->memory_monitor_.log_context_,
            &performance->log_context_);
  EXPECT_NE(&performance->js_blocking_monitor_->log_context_,
            &performance->log_context_);
  EXPECT_TRUE(LogContextEquals(performance->memory_monitor_.log_context_,
                               performance->log_context_));
  EXPECT_TRUE(LogContextEquals(performance->js_blocking_monitor_->log_context_,
                               performance->log_context_));

  // SetNativeFacade() test
  EXPECT_EQ(reinterpret_cast<intptr_t>(shell_->facade_actor_->Impl()), facade_);

  auto out_lynx_engine = shell_->engine_actor_->Impl();
  // SetPaintingContextCreator() test
  EXPECT_EQ(reinterpret_cast<intptr_t>(out_lynx_engine->GetTasm()
                                           ->page_proxy()
                                           ->element_manager()
                                           ->catalyzer()
                                           ->painting_context()
                                           ->platform_impl_.get()),
            painting_context_);
  // SetLynxEnvConfig() test
  EXPECT_EQ(out_lynx_engine->GetTasm()
                ->page_proxy()
                ->element_manager()
                ->GetLynxEnvConfig()
                .ScreenWidth(),
            lynx_env_config_->ScreenWidth());
  EXPECT_EQ(out_lynx_engine->GetTasm()
                ->page_proxy()
                ->element_manager()
                ->GetLynxEnvConfig()
                .ScreenHeight(),
            lynx_env_config_->ScreenHeight());
  // SetLazyBundleLoader() test
  EXPECT_EQ(out_lynx_engine->GetTasm()->component_loader_.get(), loader_.get());
  // SetEnablePreUpdateData() test
  EXPECT_EQ(out_lynx_engine->GetTasm()->enable_pre_update_data_,
            enable_pre_update_data_);
  // SetEnableLayoutOnly() test
  EXPECT_EQ(out_lynx_engine->GetTasm()
                ->page_proxy()
                ->element_manager()
                ->GetEnableLayoutOnly(),
            enable_layout_only_);
  // SetTasmLocale() test
  EXPECT_EQ(out_lynx_engine->GetTasm()->locale_, locale_);

  // SetLayoutContextPlatformImpl() test
  auto out_layout_context = shell_->layout_actor_->Impl();
  EXPECT_EQ(out_layout_context->page_options_.GetInstanceID(),
            shell_->instance_id_);
  EXPECT_EQ(out_layout_context->platform_impl_.get(), nullptr);

  // SetStrategy() test
  EXPECT_EQ(shell_->ThreadStrategy(), strategy_);

  EXPECT_EQ(shell_->layout_result_manager_, nullptr);
  EXPECT_EQ(shell_->layout_mediator_->layout_result_manager_, nullptr);

  // SystemInfo.pixelRatio test
  auto& element_manager = shell_->GetTasm()->page_proxy()->element_manager();
  ASSERT_EQ(shell_->GetTasm()->GetDevicePixelRatio(), 1.0f);
  ASSERT_EQ(element_manager->GetLynxEnvConfig().DevicePixelRatio(), 1.0f);

  shell_->UpdateScreenMetrics(800, 600, 1.75f);

  fml::AutoResetWaitableEvent arwe;
  shell_->RunOnTasmThread([&arwe]() { arwe.Signal(); });
  arwe.Wait();

  ASSERT_EQ(shell_->GetTasm()->GetDevicePixelRatio(), 1.75f);
  ASSERT_EQ(element_manager->GetLynxEnvConfig().DevicePixelRatio(), 1.75f);
}

#if ENABLE_TESTBENCH_RECORDER
TEST_F(LynxShellBuilderTest, RecorderIdIsInitializedWithoutJSRuntime) {
  option_->enable_js_ = false;
  auto facade = std::make_unique<MockNativeFacade>();
  auto painting_context_creator = [](LynxShell*) {
    return std::make_unique<tasm::PaintingContextPlatformImpl>();
  };

  shell_.reset((*shell_builder_)
                   .SetNativeFacade(std::move(facade))
                   .SetPaintingContextCreator(painting_context_creator)
                   .SetLynxEnvConfig(*lynx_env_config_)
                   .SetLazyBundleLoader(loader_)
                   .SetLayoutContextPlatformImpl(nullptr)
                   .SetStrategy(strategy_)
                   .SetShellOption(*option_)
                   .SetNativeModuleManager(
                       std::make_unique<pub::LynxNativeModuleManager>())
                   .build());

  ASSERT_NE(shell_, nullptr);
  EXPECT_FALSE(shell_->IsRuntimeEnabled());
  EXPECT_EQ(shell_->GetRuntimeActor(), nullptr);

  const int64_t record_id = reinterpret_cast<int64_t>(shell_.get());
  ASSERT_EQ(shell_->GetTasm()->GetRecordID(), record_id);
  EXPECT_EQ(shell_->layout_actor_->Impl()->record_id_, record_id);
  ASSERT_NE(shell_->GetTasm()->lepus_module_manager_, nullptr);
  EXPECT_EQ(shell_->GetTasm()->lepus_module_manager_->record_id_, record_id);
}
#endif

TEST_F(LynxShellBuilderTest, InitRuntimePublishesCompleteLogContext) {
  auto facade = std::make_unique<MockNativeFacade>();
  auto* facade_ptr = facade.get();
  auto painting_context =
      std::make_unique<lynx::tasm::PaintingContextPlatformImpl>();
  auto painting_context_creator = [&](lynx::shell::LynxShell*) {
    return std::move(painting_context);
  };
  shell_.reset((*shell_builder_)
                   .SetNativeFacade(std::move(facade))
                   .SetPaintingContextCreator(painting_context_creator)
                   .SetLynxEnvConfig(*lynx_env_config_)
                   .SetLazyBundleLoader(loader_)
                   .SetLayoutContextPlatformImpl(nullptr)
                   .SetStrategy(strategy_)
                   .SetShellOption(*option_)
                   .build());

  shell_->InitRuntime(
      "group", nullptr, nullptr,
      [](const std::shared_ptr<LynxActor<BTSRuntime>>&) {}, {},
      LynxRuntimeFlags::PENDING_JS_TASK, "");

  const auto context = shell_->GetLogContextSnapshot();
  EXPECT_NE(context.runtime_id, base::kUnavailableLynxEntityId);
  EXPECT_EQ(shell_->engine_actor_->ActSync([](auto& engine) {
    return engine->GetLogContext().runtime_id;
  }),
            context.runtime_id);
  EXPECT_EQ(shell_->engine_actor_->ActSync([](auto& engine) {
    return engine->GetTasm()->GetLogContext().runtime_id;
  }),
            context.runtime_id);
  EXPECT_TRUE(shell_->engine_actor_->ActSync([context](auto& engine) {
    return EngineTreeLogContextEquals(engine.get(), context);
  }));
  EXPECT_EQ(shell_->layout_actor_->ActSync([](auto& layout) {
    return layout->GetLogContext().runtime_id;
  }),
            context.runtime_id);
  EXPECT_EQ(shell_->runtime_actor_->ActSync([](auto& runtime) {
    return runtime->GetLogContext().runtime_id;
  }),
            context.runtime_id);
  EXPECT_EQ(shell_->perf_controller_actor_->ActSync([](auto& controller) {
    return controller->GetLogContext().runtime_id;
  }),
            context.runtime_id);
  EXPECT_TRUE(
      shell_->perf_controller_actor_->ActSync([context](auto& controller) {
        return LogContextEquals(controller->GetLogContext(), context) &&
               LogContextEquals(controller->memory_monitor_.log_context_,
                                context) &&
               LogContextEquals(controller->js_blocking_monitor_->log_context_,
                                context);
      }));
  shell_->facade_actor_->ActSync([](auto&) {});
  const auto updates = facade_ptr->GetLogContextUpdates();
  ASSERT_EQ(updates.size(), 1u);
  EXPECT_EQ(updates[0].view_id, context.view_id);
  EXPECT_EQ(updates[0].engine_id, context.engine_id);
  EXPECT_EQ(updates[0].runtime_id, context.runtime_id);
}

TEST_F(LynxShellBuilderTest, AttachRuntimePublishesCreationContext) {
  auto facade = std::make_unique<MockNativeFacade>();
  auto* facade_ptr = facade.get();
  auto painting_context =
      std::make_unique<lynx::tasm::PaintingContextPlatformImpl>();
  auto painting_context_creator = [&](lynx::shell::LynxShell*) {
    return std::move(painting_context);
  };
  auto runtime_actor = std::make_shared<LynxActor<BTSRuntime>>(
      nullptr, MockRunnerManufactor::GetHookJsTaskRunner(), kUnknownInstanceId,
      false);
  base::LogContext creation_context;
  creation_context.runtime_id = GenerateLynxEntityId();
  shell_.reset((*shell_builder_)
                   .SetNativeFacade(std::move(facade))
                   .SetPaintingContextCreator(painting_context_creator)
                   .SetLynxEnvConfig(*lynx_env_config_)
                   .SetLazyBundleLoader(loader_)
                   .SetLayoutContextPlatformImpl(nullptr)
                   .SetStrategy(strategy_)
                   .SetShellOption(*option_)
                   .SetRuntimeActor(runtime_actor)
                   .SetRuntimeCreationContext(&creation_context)
                   .build());

  shell_->AttachRuntime();

  const auto context = shell_->GetLogContextSnapshot();
  EXPECT_EQ(context.runtime_id, creation_context.runtime_id);
  EXPECT_FALSE(shell_->runtime_creation_context_);
  EXPECT_EQ(shell_->engine_actor_->ActSync([](auto& engine) {
    return engine->GetLogContext().runtime_id;
  }),
            creation_context.runtime_id);
  EXPECT_EQ(shell_->layout_actor_->ActSync([](auto& layout) {
    return layout->GetLogContext().runtime_id;
  }),
            creation_context.runtime_id);
  EXPECT_EQ(shell_->perf_controller_actor_->ActSync([](auto& controller) {
    return controller->GetLogContext().runtime_id;
  }),
            creation_context.runtime_id);
  shell_->facade_actor_->ActSync([](auto&) {});
  const auto updates = facade_ptr->GetLogContextUpdates();
  ASSERT_EQ(updates.size(), 1u);
  EXPECT_TRUE(LogContextEquals(updates.back(), context));
}

TEST_F(LynxShellBuilderTest,
       AttachRuntimeWithoutCreationContextPreservesLegacyContext) {
  auto facade = std::make_unique<MockNativeFacade>();
  auto painting_context =
      std::make_unique<lynx::tasm::PaintingContextPlatformImpl>();
  auto painting_context_creator = [&](lynx::shell::LynxShell*) {
    return std::move(painting_context);
  };
  auto runtime_actor = std::make_shared<LynxActor<BTSRuntime>>(
      nullptr, MockRunnerManufactor::GetHookJsTaskRunner(), kUnknownInstanceId,
      false);
  shell_.reset((*shell_builder_)
                   .SetNativeFacade(std::move(facade))
                   .SetPaintingContextCreator(painting_context_creator)
                   .SetLynxEnvConfig(*lynx_env_config_)
                   .SetLazyBundleLoader(loader_)
                   .SetLayoutContextPlatformImpl(nullptr)
                   .SetStrategy(strategy_)
                   .SetShellOption(*option_)
                   .SetRuntimeActor(runtime_actor)
                   .build());

  ASSERT_FALSE(shell_->runtime_creation_context_);
  shell_->AttachRuntime();

  EXPECT_EQ(shell_->runtime_actor_, runtime_actor);
  EXPECT_EQ(shell_->GetLogContextSnapshot().runtime_id,
            base::kUnavailableLynxEntityId);
}

TEST_F(LynxShellBuilderTest,
       LynxShellBuilderDisableForceLayoutOnBackgroundThreadTest) {
  auto facade = std::make_unique<MockNativeFacade>();
  facade_ = reinterpret_cast<intptr_t>(facade.get());

  auto painting_context =
      std::make_unique<lynx::tasm::PaintingContextPlatformImpl>();
  painting_context_ = reinterpret_cast<intptr_t>(painting_context.get());
  auto painting_context_creator = [&](lynx::shell::LynxShell* shell) {
    return std::move(painting_context);
  };

  shell_.reset((*shell_builder_)
                   .SetNativeFacade(std::move(facade))
                   .SetPaintingContextCreator(painting_context_creator)
                   .SetLynxEnvConfig(*lynx_env_config_)
                   .SetEnableElementManagerVsyncMonitor(true)
                   .SetLazyBundleLoader(loader_)
                   .SetEnablePreUpdateData(enable_pre_update_data_)
                   .SetEnableLayoutOnly(enable_layout_only_)
                   .SetTasmLocale(locale_)
                   .SetLayoutContextPlatformImpl(nullptr)
                   .SetStrategy(strategy_)
                   .SetShellOption(*option_)
                   .SetForceLayoutOnBackgroundThread(false)
                   .build());

  shell_->runtime_actor_ = std::make_shared<LynxActor<BTSRuntime>>(
      nullptr, shell_->runners_.GetUITaskRunner());

  EXPECT_EQ(shell_->layout_result_manager_, nullptr);
  EXPECT_EQ(shell_->layout_mediator_->layout_result_manager_, nullptr);
}

TEST_F(LynxShellBuilderTest,
       LynxShellBuilderEnableForceLayoutOnBackgroundThreadTest) {
  auto facade = std::make_unique<MockNativeFacade>();
  facade_ = reinterpret_cast<intptr_t>(facade.get());

  auto painting_context =
      std::make_unique<lynx::tasm::PaintingContextPlatformImpl>();
  painting_context_ = reinterpret_cast<intptr_t>(painting_context.get());
  auto painting_context_creator = [&](lynx::shell::LynxShell* shell) {
    return std::move(painting_context);
  };

  shell_.reset((*shell_builder_)
                   .SetNativeFacade(std::move(facade))
                   .SetPaintingContextCreator(painting_context_creator)
                   .SetLynxEnvConfig(*lynx_env_config_)
                   .SetEnableElementManagerVsyncMonitor(true)
                   .SetLazyBundleLoader(loader_)
                   .SetEnablePreUpdateData(enable_pre_update_data_)
                   .SetEnableLayoutOnly(enable_layout_only_)
                   .SetTasmLocale(locale_)
                   .SetLayoutContextPlatformImpl(nullptr)
                   .SetStrategy(strategy_)
                   .SetShellOption(*option_)
                   .SetForceLayoutOnBackgroundThread(true)
                   .build());

  shell_->runtime_actor_ = std::make_shared<LynxActor<BTSRuntime>>(
      nullptr, shell_->runners_.GetUITaskRunner());

  EXPECT_NE(shell_->layout_result_manager_, nullptr);
  EXPECT_NE(shell_->layout_mediator_->layout_result_manager_, nullptr);

  EXPECT_EQ(shell_->layout_mediator_->layout_result_manager_,
            shell_->layout_result_manager_);
  EXPECT_EQ(shell_->layout_mediator_->layout_result_manager_,
            shell_->layout_mediator_->operation_queue_);
}

TEST_F(LynxShellBuilderTest, EngineHandoffPreservesEngineIdentity) {
  LynxEngineWrapper engine_wrapper;
  MockNativeFacade* old_facade = nullptr;
  auto old_shell =
      BuildShellForEngineHandoff(101, &engine_wrapper, &old_facade);
  const auto engine_id = old_shell->GetLogContextSnapshot().engine_id;
  EXPECT_EQ(engine_wrapper.engine_context_.view_id,
            base::kUnavailableLynxEntityId);
  EXPECT_EQ(engine_wrapper.engine_context_.engine_id, engine_id);
  EXPECT_EQ(engine_wrapper.engine_context_.runtime_id,
            base::kUnavailableLynxEntityId);

  old_shell->PrepareEngineHandoff();
  EXPECT_EQ(old_shell->GetLogContextSnapshot().engine_id,
            base::kUnavailableLynxEntityId);
  const auto detached_engine_context = engine_wrapper.engine_actor_->ActSync(
      [](auto& engine) { return engine->GetLogContext(); });
  const auto detached_layout_context = engine_wrapper.layout_actor_->ActSync(
      [](auto& layout) { return layout->GetLogContext(); });
  EXPECT_EQ(detached_engine_context.view_id, base::kUnavailableLynxEntityId);
  EXPECT_EQ(detached_engine_context.engine_id, engine_id);
  EXPECT_EQ(detached_layout_context.view_id, base::kUnavailableLynxEntityId);
  EXPECT_EQ(detached_layout_context.engine_id, engine_id);
  EXPECT_TRUE(engine_wrapper.engine_actor_->ActSync([detached_engine_context](
                                                        auto& engine) {
    return EngineTreeLogContextEquals(engine.get(), detached_engine_context);
  }));

  old_shell->facade_actor_->ActSync([](auto&) {});
  const auto old_updates = old_facade->GetLogContextUpdates();
  ASSERT_EQ(old_updates.size(), 2u);
  EXPECT_EQ(old_updates.back().view_id, 101);
  EXPECT_EQ(old_updates.back().engine_id, base::kUnavailableLynxEntityId);

  MockNativeFacade* new_facade = nullptr;
  auto new_shell = BuildShellForEngineHandoff(202, nullptr, &new_facade);
  EXPECT_NE(new_shell->GetLogContextSnapshot().engine_id, engine_id);
  new_shell->ReattachLynxEngineWrapper(&engine_wrapper);
  const auto rebound_context = new_shell->GetLogContextSnapshot();
  EXPECT_EQ(rebound_context.view_id, 202);
  EXPECT_EQ(rebound_context.engine_id, engine_id);
  EXPECT_EQ(new_shell->engine_actor_->ActSync(
                [](auto& engine) { return engine->GetLogContext().view_id; }),
            202);
  EXPECT_EQ(new_shell->layout_actor_->ActSync(
                [](auto& layout) { return layout->GetLogContext().view_id; }),
            202);
  EXPECT_TRUE(
      new_shell->engine_actor_->ActSync([rebound_context](auto& engine) {
        return EngineTreeLogContextEquals(engine.get(), rebound_context);
      }));
  new_shell->facade_actor_->ActSync([](auto&) {});
  const auto new_updates = new_facade->GetLogContextUpdates();
  ASSERT_EQ(new_updates.size(), 2u);
  EXPECT_EQ(new_updates.back().engine_id, engine_id);
}

}  // namespace shell
}  // namespace lynx
