// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/module/extension_module_factory_impl.h"

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "base/include/fml/thread.h"
#include "core/runtime/common/napi/napi_environment.h"
#include "core/runtime/common/napi/napi_runtime_proxy_quickjs.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/embedder/module/lynx_extension_module_priv.h"
#include "quickjs/include/quickjs.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace embedder {
namespace {

struct ExtensionModuleCounters {
  int creator_count = 0;
  int napi_creator_count = 0;
  int view_create_count = 0;
  int view_destroy_count = 0;
  int runtime_init_count = 0;
  int runtime_attach_count = 0;
  int runtime_ready_count = 0;
  int runtime_detach_count = 0;
  int enter_foreground_count = 0;
  int enter_background_count = 0;
  int destroy_count = 0;
  int finalizer_count = 0;
};

ExtensionModuleCounters* GetCounters(lynx_extension_module_t* module) {
  return static_cast<ExtensionModuleCounters*>(
      lynx_extension_module_get_user_data(module));
}

napi_value_weak Echo(napi_env_weak env, napi_callback_info_weak info) {
  size_t argument_count = 1;
  napi_value_weak arguments[1] = {nullptr};
  napi_get_cb_info_weak(env, info, &argument_count, arguments, nullptr,
                        nullptr);
  if (argument_count == 0 || arguments[0] == nullptr) {
    napi_value_weak undefined = nullptr;
    napi_get_undefined_weak(env, &undefined);
    return undefined;
  }
  return arguments[0];
}

lynx_extension_module_t* CreateExtensionModule(void* opaque) {
  auto* counters = static_cast<ExtensionModuleCounters*>(opaque);
  ++counters->creator_count;
  auto* module = lynx_extension_module_create_with_finalizer(
      counters, [](lynx_extension_module_t*, void* user_data) {
        ++static_cast<ExtensionModuleCounters*>(user_data)->finalizer_count;
      });
  lynx_extension_module_bind_lynx_view_create(
      module, [](lynx_extension_module_t* module, lynx_view_t*) {
        ++GetCounters(module)->view_create_count;
      });
  lynx_extension_module_bind_lynx_view_destroy(
      module, [](lynx_extension_module_t* module) {
        ++GetCounters(module)->view_destroy_count;
      });
  lynx_extension_module_bind_runtime_init(
      module, [](lynx_extension_module_t* module) {
        ++GetCounters(module)->runtime_init_count;
      });
  lynx_extension_module_bind_runtime_attach(
      module,
      [](lynx_extension_module_t* module, auto, lynx_vsync_observer_t*) {
        ++GetCounters(module)->runtime_attach_count;
      });
  lynx_extension_module_bind_runtime_ready(
      module, [](lynx_extension_module_t* module, auto, auto, const char*) {
        ++GetCounters(module)->runtime_ready_count;
      });
  lynx_extension_module_bind_runtime_detach(
      module, [](lynx_extension_module_t* module) {
        ++GetCounters(module)->runtime_detach_count;
      });
  lynx_extension_module_bind_enter_foreground(
      module, [](lynx_extension_module_t* module) {
        ++GetCounters(module)->enter_foreground_count;
      });
  lynx_extension_module_bind_enter_background(
      module, [](lynx_extension_module_t* module) {
        ++GetCounters(module)->enter_background_count;
      });
  lynx_extension_module_bind_on_destroy(module,
                                        [](lynx_extension_module_t* module) {
                                          ++GetCounters(module)->destroy_count;
                                        });
  lynx_extension_module_set_napi_module_creator(
      module, [](napi_env_weak env, napi_value_weak exports, const char*,
                 void* opaque) {
        ++static_cast<ExtensionModuleCounters*>(opaque)->napi_creator_count;
        napi_value_weak function = nullptr;
        napi_create_function_weak(env, "extensionEcho", NAPI_AUTO_LENGTH, Echo,
                                  nullptr, &function);
        napi_set_named_property_weak(env, exports, "extensionEcho", function);
        return exports;
      });
  return module;
}

class TestModuleDelegate final : public runtime::LynxNativeModule::Delegate {
 public:
  void InvokeCallback(
      const std::shared_ptr<runtime::LynxModuleCallback>& callback,
      base::MoveOnlyClosure<bool> invoke_pre_func = nullptr) override {}
  void RunOnJSThread(base::closure func) override { func(); }
  void RunOnPlatformThread(base::closure func) override { func(); }
  const std::shared_ptr<pub::PubValueFactory>& GetValueFactory() override {
    return value_factory_;
  }
  void OnErrorOccurred(const std::string& module_name,
                       const std::string& method_name,
                       base::LynxError error) override {}

 private:
  std::shared_ptr<pub::PubValueFactory> value_factory_ =
      std::make_shared<pub::PubValueFactoryDefault>();
};

class MTSExtensionModuleFactoryTest : public ::testing::Test {
 protected:
  MTSExtensionModuleFactoryTest()
      : runtime_(LEPUS_NewRuntime()), context_(LEPUS_NewContext(runtime_)) {
    auto proxy = runtime::js::NapiRuntimeProxyQuickjs::Create(context_);
    napi_environment_ = std::make_unique<runtime::js::NapiEnvironment>(
        std::make_unique<runtime::js::NapiEnvironment::Delegate>());
    napi_environment_->SetRuntimeProxy(std::move(proxy));
    napi_environment_->Attach();
  }

  ~MTSExtensionModuleFactoryTest() override {
    napi_environment_->Detach();
    LEPUS_FreeContext(context_);
    LEPUS_FreeRuntime(runtime_);
  }

  LEPUSRuntime* runtime_;
  LEPUSContext* context_;
  std::unique_ptr<runtime::js::NapiEnvironment> napi_environment_;
};

class TestVSyncObserver : public runtime::IVSyncObserver {
 public:
  using Callback = base::MoveOnlyClosure<void, int64_t, int64_t>;
  void RequestAnimationFrame(uintptr_t id, Callback callback) override {
    callbacks.emplace(id, std::move(callback));
  }
  void RequestBeforeAnimationFrame(uintptr_t id, Callback callback) override {
    callbacks.emplace(id, std::move(callback));
  }
  void RegisterAfterAnimationFrameListener(Callback callback) override {}
  std::unordered_map<uintptr_t, Callback> callbacks;
};

struct VSyncResult {
  int calls = 0;
  int64_t start = -1;
  int64_t end = -1;
  static void OnSignal(void* data, int64_t start, int64_t end) {
    auto& result = *static_cast<VSyncResult*>(data);
    ++result.calls;
    result.start = start;
    result.end = end;
  }
};

class ExtensionModuleVSyncTest : public ::testing::TestWithParam<bool> {
 protected:
  struct Context {
    VSyncResult* result;
    std::vector<std::string>* events;
    fml::TaskRunner* runner;
  };

  void SetUp() override {
    runner_ = thread_.GetTaskRunner();
    runner_->PostSyncTask([&] {
      context_ = new Context{&result_, &events_, runner_.get()};
      module_ = lynx_extension_module_create_with_finalizer(
          context_, [](auto*, void* data) {
            auto* context = static_cast<Context*>(data);
            context->events->push_back("finalizer");
            delete context;
          });
      lynx_extension_module_bind_runtime_attach(module_,
                                                [](auto*, auto, auto*) {});
      lynx_extension_module_bind_runtime_detach(module_, [](auto* module) {
        auto* context =
            static_cast<Context*>(lynx_extension_module_get_user_data(module));
        context->events->push_back("detach");
      });
      lynx_extension_module_bind_on_destroy(module_, [](auto* module) {
        auto* context =
            static_cast<Context*>(lynx_extension_module_get_user_data(module));
        context->events->push_back("destroy");
      });
      extension_ = std::make_unique<ExtensionModuleImpl>(module_);
      extension_->SetRuntimeAttachedState(nullptr, observer_);
    });
  }

  void TearDown() override {
    runner_->PostSyncTask([&] {
      extension_.reset();
      observer_->callbacks.clear();
    });
  }

 public:
  void Request(uintptr_t id = 1) {
    Request(
        id,
        [](void* data, int64_t start, int64_t end) {
          auto* context = static_cast<Context*>(data);
          EXPECT_TRUE(context->runner->RunsTasksOnCurrentThread());
          context->events->push_back(start == 0 ? "cancel" : "signal");
          VSyncResult::OnSignal(context->result, start, end);
        },
        context_);
  }

  void Request(uintptr_t id, vsync_observer_callback callback, void* data) {
    auto request = GetParam()
                       ? lynx_vsync_observer_request_before_animation_frame
                       : lynx_vsync_observer_request_animation_frame;
    request(module_->vsync_observer, id, callback, data);
  }

 protected:
  fml::Thread thread_{"extension-vsync-test"};
  fml::RefPtr<fml::TaskRunner> runner_;
  std::shared_ptr<TestVSyncObserver> observer_ =
      std::make_shared<TestVSyncObserver>();
  VSyncResult result_;
  std::vector<std::string> events_;
  Context* context_ = nullptr;
  lynx_extension_module_t* module_ = nullptr;
  std::unique_ptr<ExtensionModuleImpl> extension_;
};

TEST_P(ExtensionModuleVSyncTest, CancelsBeforeRuntimeTeardownHooks) {
  runner_->PostSyncTask([&] {
    Request();
    extension_->SetRuntimeDetachedState();
    EXPECT_EQ(events_, (std::vector<std::string>{"cancel", "detach"}));
    VSyncResult detached;
    Request(2, VSyncResult::OnSignal, &detached);
    EXPECT_EQ(detached.calls, 1);
    EXPECT_EQ(detached.start, 0);
    EXPECT_EQ(detached.end, 0);
    extension_->Destroy();
    extension_.reset();
    EXPECT_EQ(events_, (std::vector<std::string>{"cancel", "detach", "destroy",
                                                 "finalizer"}));
    // The runtime can retain a callback after its module's context is freed.
    observer_->callbacks.at(1)(100, 200);
    EXPECT_EQ(result_.calls, 1);
    EXPECT_EQ(result_.start, 0);
    EXPECT_EQ(result_.end, 0);
  });
}

TEST_P(ExtensionModuleVSyncTest, CancelsBeforeFinalizerWithSharedObserver) {
  runner_->PostSyncTask([&] {
    Request();
    extension_.reset();
    EXPECT_EQ(events_, (std::vector<std::string>{"cancel", "finalizer"}));
    observer_->callbacks.at(1)(100, 200);
    EXPECT_EQ(result_.calls, 1);
    EXPECT_EQ(result_.start, 0);
    EXPECT_EQ(result_.end, 0);
  });
}

TEST_P(ExtensionModuleVSyncTest, CancelsBeforeDestroyHookWithoutDetach) {
  runner_->PostSyncTask([&] {
    Request();
    extension_->Destroy();
    EXPECT_EQ(events_, (std::vector<std::string>{"cancel", "destroy"}));
    extension_.reset();
    observer_->callbacks.at(1)(100, 200);
    EXPECT_EQ(result_.calls, 1);
    EXPECT_EQ(events_,
              (std::vector<std::string>{"cancel", "destroy", "finalizer"}));
  });
}

TEST_P(ExtensionModuleVSyncTest, DeliveredCallbacksAreNotCancelledAgain) {
  runner_->PostSyncTask([&] {
    Request();
    auto node = observer_->callbacks.extract(1);
    node.mapped()(100, 200);
    extension_.reset();
    EXPECT_EQ(result_.calls, 1);
    EXPECT_EQ(result_.start, 100);
    EXPECT_EQ(result_.end, 200);
    EXPECT_EQ(events_, (std::vector<std::string>{"signal", "finalizer"}));
  });
}

TEST_P(ExtensionModuleVSyncTest, CancelsRejectedDuplicateCallbacks) {
  runner_->PostSyncTask([&] {
    VSyncResult duplicate;
    Request();
    Request(1, VSyncResult::OnSignal, &duplicate);
    EXPECT_EQ(result_.calls, 0);
    EXPECT_EQ(duplicate.calls, 1);
    EXPECT_EQ(duplicate.start, 0);
    EXPECT_EQ(duplicate.end, 0);
    extension_.reset();
    EXPECT_EQ(result_.calls, 1);
    EXPECT_EQ(duplicate.calls, 1);
  });
}

TEST_P(ExtensionModuleVSyncTest, CancellingOneModulePreservesOtherRequests) {
  runner_->PostSyncTask([&] {
    ExtensionModuleCounters counters;
    VSyncResult other_result;
    auto* other_module = CreateExtensionModule(&counters);
    ExtensionModuleImpl other(other_module);
    other.SetRuntimeAttachedState(nullptr, observer_);
    auto request = GetParam()
                       ? lynx_vsync_observer_request_before_animation_frame
                       : lynx_vsync_observer_request_animation_frame;
    Request();
    request(other_module->vsync_observer, 2, VSyncResult::OnSignal,
            &other_result);
    extension_.reset();
    EXPECT_EQ(result_.calls, 1);
    EXPECT_EQ(other_result.calls, 0);
    observer_->callbacks.at(2)(100, 200);
    EXPECT_EQ(other_result.calls, 1);
    EXPECT_EQ(other_result.start, 100);
    EXPECT_EQ(other_result.end, 200);
  });
}

TEST_P(ExtensionModuleVSyncTest, ReentrantRequestDuringCancellationIsRejected) {
  runner_->PostSyncTask([&] {
    struct ReentrantRequest {
      ExtensionModuleVSyncTest* test;
      VSyncResult cancelled;
      VSyncResult reentrant;
    } state{this, {}, {}};
    Request(
        1,
        [](void* data, int64_t start, int64_t end) {
          auto* state = static_cast<ReentrantRequest*>(data);
          VSyncResult::OnSignal(&state->cancelled, start, end);
          state->test->Request(2, VSyncResult::OnSignal, &state->reentrant);
        },
        &state);
    extension_->SetRuntimeDetachedState();
    EXPECT_EQ(state.cancelled.calls, 1);
    EXPECT_EQ(state.reentrant.calls, 1);
    EXPECT_EQ(state.reentrant.start, 0);
    EXPECT_EQ(state.reentrant.end, 0);
    EXPECT_EQ(observer_->callbacks.size(), 1u);
    extension_.reset();
    observer_->callbacks.clear();
    EXPECT_EQ(state.cancelled.calls, 1);
    EXPECT_EQ(state.reentrant.calls, 1);
  });
}

INSTANTIATE_TEST_SUITE_P(OneShot, ExtensionModuleVSyncTest, ::testing::Bool());

TEST_F(MTSExtensionModuleFactoryTest,
       CreatesNapiModuleWithoutExtensionLifecycle) {
  ExtensionModuleCounters counters;
  std::unordered_map<std::string,
                     std::tuple<extension_module_creator, bool, void*>>
      creators;
  creators.emplace("TestExtensionModule",
                   std::make_tuple(CreateExtensionModule, false, &counters));
  LynxMTSExtensionModuleFactoryNAPI factory(std::move(creators));
  factory.AttachOpaqueContext(napi_environment_->proxy()->Env());

  auto module = factory.CreateModule("TestExtensionModule");
  ASSERT_NE(module, nullptr);
  EXPECT_NE(module->GetMethodList().find("extensionEcho"),
            module->GetMethodList().end());

  auto delegate = std::make_shared<TestModuleDelegate>();
  module->SetDelegate(delegate);
  pub::PubValueFactoryDefault value_factory;
  auto arguments = value_factory.CreateArray();
  arguments->PushStringToArray("extension works");
  runtime::CallbackMap callbacks;
  auto result =
      module->InvokeMethod("extensionEcho", std::move(arguments), 1, callbacks);
  ASSERT_TRUE(result.has_value());
  ASSERT_NE(result.value(), nullptr);
  EXPECT_TRUE(result.value()->IsString());
  EXPECT_EQ(result.value()->str(), "extension works");

  module.reset();
  factory.DetachOpaqueContext(napi_environment_->proxy()->Env());

  EXPECT_EQ(counters.creator_count, 1);
  EXPECT_EQ(counters.napi_creator_count, 1);
  EXPECT_EQ(counters.view_create_count, 0);
  EXPECT_EQ(counters.view_destroy_count, 0);
  EXPECT_EQ(counters.runtime_init_count, 0);
  EXPECT_EQ(counters.runtime_attach_count, 0);
  EXPECT_EQ(counters.runtime_ready_count, 0);
  EXPECT_EQ(counters.runtime_detach_count, 0);
  EXPECT_EQ(counters.enter_foreground_count, 0);
  EXPECT_EQ(counters.enter_background_count, 0);
  EXPECT_EQ(counters.destroy_count, 0);
  EXPECT_EQ(counters.finalizer_count, 1);
}

TEST_F(MTSExtensionModuleFactoryTest, RequiresAttachedContext) {
  ExtensionModuleCounters counters;
  std::unordered_map<std::string,
                     std::tuple<extension_module_creator, bool, void*>>
      creators;
  creators.emplace("TestExtensionModule",
                   std::make_tuple(CreateExtensionModule, true, &counters));
  LynxMTSExtensionModuleFactoryNAPI factory(std::move(creators));

  EXPECT_EQ(factory.CreateModule("TestExtensionModule"), nullptr);
  factory.AttachOpaqueContext(napi_environment_->proxy()->Env());
  EXPECT_EQ(factory.CreateModule("UnknownModule"), nullptr);
  factory.Detach();
  EXPECT_EQ(factory.CreateModule("TestExtensionModule"), nullptr);
  EXPECT_EQ(counters.creator_count, 0);
}

}  // namespace
}  // namespace embedder
}  // namespace lynx
