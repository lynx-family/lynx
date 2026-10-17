// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/bindings/modules/host_script_module_interceptor.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/inspector/observer/native_module_record_observer.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module.h"
#include "core/runtime/js/bindings/modules/lynx_module_manager.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::runtime::js {
namespace {
using Kind = shell::InterceptKind;
lepus::Value Array(lepus::Value value) {
  auto array = lepus::CArray::Create();
  array->push_back(std::move(value));
  return lepus::Value(array);
}
#if ENABLE_INSPECTOR
shell::InterceptResult Patch(const char* key, lepus::Value value) {
  shell::InterceptResult result;
  result.patch = lepus::Value(lepus::Dictionary::Create());
  result.patch.SetProperty(key, std::move(value));
  return result;
}
#endif
class Provider : public shell::Interceptor {
 public:
  bool HasHandlers(Kind kind) const override {
    ++queries;
    return enabled && (kind == Kind::kCall || kind == Kind::kResult ||
                       kind == Kind::kCallback);
  }
  shell::InterceptResult Dispatch(Kind kind,
                                  const lepus::Value& event) override {
    events.emplace_back(kind, event);
    return dispatch ? dispatch(kind, event) : shell::InterceptResult{};
  }
  void ReportError(const std::string&) override {}
  bool enabled = true;
  mutable int queries = 0;
  std::function<shell::InterceptResult(Kind, const lepus::Value&)> dispatch;
  std::vector<std::pair<Kind, lepus::Value>> events;
};
// Exercise the same legacy hooks and short-circuit contract as
// RequestInterceptor.
class LegacyInterceptor : public ModuleInterceptor {
 public:
  explicit LegacyInterceptor(std::vector<std::string>& events)
      : events(events) {}
  ModuleInterceptorResult InterceptModuleMethod(
      const std::shared_ptr<LynxModule>&, const LynxModule::MethodMetadata&,
      Runtime* rt, const std::shared_ptr<ModuleDelegate>&, const Value* args,
      size_t count, const std::unique_ptr<pub::Value>& pub_args,
      const CallbackMap& callbacks,
      NativeModuleInfoCollectorPtr) const override {
    events.push_back("legacy.call");
    if (count && args[0].isNumber()) {
      EXPECT_EQ(args[0].getNumber(), pub_args->GetValueAtIndex(0)->Number());
    }
    if (on_call) on_call(callbacks);
    return {handled, Value(17)};
  }
  void BeforeInvokeMethod(const LynxModule::MethodMetadata&,
                          const std::unique_ptr<pub::Value>&,
                          const NativeModuleInfoCollectorPtr&) override {
    events.push_back("legacy.before");
  }
  void OnCallbackInvoked(const NativeModuleInfoCollectorPtr&,
                         ModuleCallback*) override {
    events.push_back("legacy.callback");
  }
  void SetTemplateUrl(const std::string& value) override { url = value; }
  bool handled = false;
  std::string url;
  std::function<void(const CallbackMap&)> on_call;
  std::vector<std::string>& events;
};
class NativeModule : public LynxNativeModule {
 public:
  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string&, std::unique_ptr<pub::Value> args, size_t,
      const CallbackMap& callbacks) override {
    ++calls;
    arguments = pub::ValueUtils::ConvertValueToLepusValue(*args);
    saved_callbacks = callbacks;
    if (invoke) invoke();
    if (fail) return base::unexpected(std::string("native failure"));
    return std::unique_ptr<pub::Value>(
        new pub::ValueImplLepus(lepus::Value(42)));
  }
  int calls = 0;
  bool fail = false;
  lepus::Value arguments;
  CallbackMap saved_callbacks;
  std::function<void()> invoke;
};
class Delegate : public ModuleDelegate {
 public:
  explicit Delegate(Runtime& rt) : rt(rt) {}
  void OnErrorOccurred(base::LynxError) override {}
  void OnMethodInvoked(const std::string&, const std::string&,
                       int32_t) override {}
  void FlushJSBTiming(NativeModuleInfo) override {}
  void RunOnJSThread(base::closure task) override { task(); }
  void RunOnPlatformThread(base::closure task) override { task(); }
  int64_t RegisterJSCallbackFunction(Function function) override {
    functions.push_back(
        std::make_unique<ModuleCallbackFunctionHolder>(std::move(function)));
    return functions.size() - 1;
  }
  void CallJSCallback(const std::shared_ptr<ModuleCallback>& callback,
                      base::MoveOnlyClosure<bool> = nullptr,
                      int64_t = ModuleCallback::kInvalidCallbackId) override {
    // Queue deliveries like a platform hop, retaining each callback's own args.
    pending.push_back(callback);
  }
  void Drain() {
    auto deliveries = std::move(pending);
    pending.clear();
    for (auto& callback : deliveries)
      callback->Invoke(&rt, functions[callback->callback_id()].get());
  }
  Runtime& rt;
  std::vector<std::unique_ptr<ModuleCallbackFunctionHolder>> functions;
  std::vector<std::shared_ptr<ModuleCallback>> pending;
};
struct Recorded {
  lepus::Value args;
  std::optional<lepus::Value> result;
  bool success = false;
  int invokes = 0;
  std::vector<std::pair<int, lepus::Value>> callbacks;
};
class RecordContext : public NativeModuleInvocationContext {
 public:
  explicit RecordContext(std::shared_ptr<Recorded> data, int index = -1)
      : data(data), index(index) {}
  std::shared_ptr<NativeModuleInvocationContext> WithCallbackArgumentIndex(
      int32_t index) const override {
    return std::make_shared<RecordContext>(data, index);
  }
  void OnInvoke(lepus::Value args, const CallbackMap&, bool success,
                std::optional<lepus::Value> result, int32_t,
                const std::string&) const override {
    ++data->invokes;
    data->args = args;
    data->success = success;
    data->result = result;
  }
  void OnCallback(lepus::Value value) const override {
    data->callbacks.emplace_back(index, value);
  }
  std::shared_ptr<Recorded> data;
  int index;
};
class Observer : public NativeModuleRecordObserver {
 public:
  void OnRecord(const lepus::Value&) override {}
  void OnGlobalEvent(const std::string&, const lepus::Value&) override {}
  std::shared_ptr<NativeModuleInvocationContext> CreateInvocation(
      const std::string&, const std::string&) override {
    return std::make_shared<RecordContext>(data);
  }
  std::shared_ptr<Recorded> data = std::make_shared<Recorded>();
};
class HostScriptModuleInterceptorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto vm = rt.createVM(nullptr);
    auto context = rt.createContext(vm);
    rt.InitRuntime(context);
    ASSERT_TRUE(
        shell::Interceptor::Attach(provider, shell::Interceptor::Domain::kBTS));
    shell::Interceptor::RegisterView(kView);
    auto legacy_ptr = std::make_unique<LegacyInterceptor>(events);
    legacy = legacy_ptr.get();
    group->AddInterceptor(std::move(legacy_ptr));
    group->AddInterceptor(std::make_unique<HostScriptModuleInterceptor>(),
                          true);
    module->SetLogContext({kView, 0, 0});
    module->SetModuleInterceptor(group);
    native->SetDelegate(module);
    native->invoke = [&] { events.push_back("native"); };
  }
  void TearDown() override {
    EXPECT_EQ(GroupInterceptor::Current(), nullptr);
    delegate->pending.clear();
    shell::Interceptor::DestroyView(kView);
    shell::Interceptor::Detach(provider.get());
  }
  auto Call(const Value* args = nullptr, size_t count = 0,
            const std::string& method = "echo") {
    return module->invokeMethod(LynxModule::MethodMetadata(count, method), &rt,
                                args, count);
  }
  Value Callback(std::vector<double>& values) {
    return Value(Function::createFromHostFunction(
        rt, PropNameID::forAscii(rt, "callback"), 1,
        [&values](Runtime&, const Value&, const Value* args, size_t count) {
          if (count) values.push_back(args[0].getNumber());
          return Value::undefined();
        }));
  }
  void Deliver(const std::shared_ptr<LynxModuleCallback>& callback, int value) {
    callback->SetArgs(
        std::make_unique<pub::ValueImplLepus>(Array(lepus::Value(value))));
    module->InvokeCallback(callback);
  }
  static constexpr base::LynxEntityId kView = 42;
  QuickjsRuntime rt;
  std::vector<std::string> events;
  std::shared_ptr<Provider> provider = std::make_shared<Provider>();
  std::shared_ptr<Delegate> delegate = std::make_shared<Delegate>(rt);
  std::shared_ptr<NativeModule> native = std::make_shared<NativeModule>();
  std::shared_ptr<LynxJSIModule> module =
      std::make_shared<LynxJSIModule>("Example", delegate, native);
  std::shared_ptr<GroupInterceptor> group =
      std::make_shared<GroupInterceptor>();
  LegacyInterceptor* legacy = nullptr;
};
TEST_F(HostScriptModuleInterceptorTest, InspectorGateAndIdleFastPath) {
  EXPECT_EQ(group->CreateInvocationGroup("Example", "echo", kView) != nullptr,
            bool(ENABLE_INSPECTOR));
  provider->enabled = false;
  EXPECT_EQ(group->CreateInvocationGroup("Example", "echo", kView), nullptr);
  EXPECT_TRUE(Call().has_value());
  EXPECT_TRUE(provider->events.empty());
  EXPECT_EQ(events, (std::vector<std::string>{"legacy.before", "legacy.call",
                                              "native"}));
}
TEST_F(HostScriptModuleInterceptorTest, LegacyShortCircuitAndUrlPropagation) {
  legacy->handled = true;
  auto second = std::make_unique<LegacyInterceptor>(events);
  group->AddInterceptor(std::move(second));
  group->SetTemplateUrl("test://page");
  EXPECT_EQ(legacy->url, "test://page");
  auto result = Call();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 17);
  EXPECT_EQ(native->calls, 0);
  EXPECT_EQ(events, (std::vector<std::string>{"legacy.before", "legacy.before",
                                              "legacy.call"}));
}
TEST_F(HostScriptModuleInterceptorTest, LegacyCallbackNotificationPreserved) {
  std::vector<double> values;
  Value args[] = {Callback(values)};
  legacy->handled = true;
  legacy->on_call = [&](const CallbackMap& callbacks) {
    Deliver(callbacks.at(0), 7);
  };
  ASSERT_TRUE(Call(args, 1).has_value());
  delegate->Drain();
  EXPECT_EQ(values, (std::vector<double>{7}));
  EXPECT_EQ(events, (std::vector<std::string>{"legacy.before", "legacy.call",
                                              "legacy.callback"}));
  EXPECT_EQ(native->calls, 0);
}
TEST_F(HostScriptModuleInterceptorTest, NativeFailureKeepsErrorAndRecording) {
  auto observer = std::make_shared<Observer>();
  module->SetNativeModuleRecordObserver(observer);
  native->fail = true;
  EXPECT_FALSE(Call().has_value());
  EXPECT_EQ(observer->data->invokes, 1);
  EXPECT_FALSE(observer->data->success);
  EXPECT_FALSE(observer->data->result.has_value());
  for (auto& event : provider->events) EXPECT_NE(event.first, Kind::kResult);
}
TEST_F(HostScriptModuleInterceptorTest, ManagerRegistersHostScript) {
  class Manager : public LynxModuleManager {
   public:
    using LynxModuleManager::GetModule;
  } manager;
  manager.SetLogContext({kView, 0, 0});
  auto factory = std::make_unique<NativeModuleFactory>();
  factory->Register("Example", [&] { return native; });
  manager.SetModuleFactory(std::move(factory));
  manager.InitModuleInterceptor();
  auto created = manager.GetModule("Example", delegate);
  ASSERT_NE(created, nullptr);
  ASSERT_TRUE(
      created
          ->invokeMethod(LynxModule::MethodMetadata(0, "echo"), &rt, nullptr, 0)
          .has_value());
  EXPECT_EQ(!provider->events.empty(), bool(ENABLE_INSPECTOR));
}

TEST_F(HostScriptModuleInterceptorTest, RecordingWorksWithoutHostScript) {
  shell::Interceptor::Detach(provider.get());
  auto observer = std::make_shared<Observer>();
  module->SetNativeModuleRecordObserver(observer);
  std::vector<double> values;
  Value args[] = {Callback(values)};
  auto result = Call(args, 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 42);
  Deliver(native->saved_callbacks.at(0), 7);
  delegate->Drain();
  EXPECT_TRUE(provider->events.empty());
  ASSERT_TRUE(observer->data->result);
  EXPECT_EQ(observer->data->result->Number(), 42);
  ASSERT_EQ(observer->data->callbacks.size(), 1u);
  EXPECT_EQ(observer->data->callbacks[0].second.GetProperty(0).Number(), 7);
}
#if ENABLE_INSPECTOR
TEST_F(HostScriptModuleInterceptorTest,
       RewritesBeforeConversionAndRecordsEffectiveResult) {
  auto observer = std::make_shared<Observer>();
  module->SetNativeModuleRecordObserver(observer);
  provider->dispatch = [&](Kind kind, const lepus::Value&) {
    events.push_back(kind == Kind::kCall ? "host.call" : "host.result");
    return kind == Kind::kCall ? Patch("args", Array(lepus::Value(9)))
                               : Patch("value", lepus::Value(99));
  };
  Value args[] = {Value(1)};
  auto result = Call(args, 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 99);
  EXPECT_EQ(native->arguments.GetProperty(0).Number(), 9);
  EXPECT_EQ(args[0].getNumber(), 1);
  EXPECT_EQ(observer->data->args.GetProperty(0).Number(), 9);
  ASSERT_TRUE(observer->data->result);
  EXPECT_EQ(observer->data->result->Number(), 99);
  EXPECT_EQ(events,
            (std::vector<std::string>{"host.call", "legacy.before",
                                      "legacy.call", "native", "host.result"}));
  EXPECT_EQ(provider->events[0].second.GetProperty("invocationId"),
            provider->events[1].second.GetProperty("invocationId"));
}
TEST_F(HostScriptModuleInterceptorTest,
       MockSkipsNetworkAndNativeAndKeepsRepeatedDeliveries) {
  auto observer = std::make_shared<Observer>();
  module->SetNativeModuleRecordObserver(observer);
  provider->dispatch = [](Kind kind, const lepus::Value&) {
    if (kind != Kind::kCall) return shell::InterceptResult{};
    shell::InterceptResult decision;
    decision.mock = lepus::Value(lepus::Dictionary::Create());
    decision.mock.SetProperty("returnValue", lepus::Value());
    auto deliveries = lepus::CArray::Create();
    for (int value : {10, 20}) {
      auto item = lepus::Value(lepus::Dictionary::Create());
      item.SetProperty("argumentIndex", lepus::Value(0));
      item.SetProperty("args", Array(lepus::Value(value)));
      deliveries->push_back(item);
    }
    decision.mock.SetProperty("callbacks", lepus::Value(deliveries));
    return decision;
  };
  std::vector<double> values;
  Value args[] = {Callback(values)};
  auto result = Call(args, 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->isNull());
  ASSERT_EQ(delegate->pending.size(), 2u);
  EXPECT_NE(delegate->pending[0]->callback_id(),
            delegate->pending[1]->callback_id());
  delegate->Drain();
  EXPECT_EQ(values, (std::vector<double>{10, 20}));
  EXPECT_TRUE(events.empty());
  EXPECT_EQ(native->calls, 0);
  ASSERT_TRUE(observer->data->result);
  EXPECT_TRUE(observer->data->result->IsNil());
  ASSERT_EQ(observer->data->callbacks.size(), 2u);
  EXPECT_EQ(observer->data->callbacks[0].first, 0);
  EXPECT_EQ(observer->data->callbacks[1].second.GetProperty(0).Number(), 20);
}
TEST_F(HostScriptModuleInterceptorTest, MockWithoutRecordingObserver) {
  provider->dispatch = [](Kind kind, const lepus::Value&) {
    shell::InterceptResult result;
    if (kind == Kind::kCall) {
      result.mock = lepus::Value(lepus::Dictionary::Create());
      auto item = lepus::Value(lepus::Dictionary::Create());
      item.SetProperty("argumentIndex", lepus::Value(0));
      item.SetProperty("args", Array(lepus::Value(5)));
      result.mock.SetProperty("callbacks", Array(item));
    }
    return result;
  };
  std::vector<double> values;
  Value args[] = {Callback(values)};
  auto result = Call(args, 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->isUndefined());
  delegate->Drain();
  EXPECT_EQ(values, (std::vector<double>{5}));
  EXPECT_TRUE(events.empty());
}
TEST_F(HostScriptModuleInterceptorTest, CallbackRewriteBeforeJSAndRecording) {
  auto observer = std::make_shared<Observer>();
  module->SetNativeModuleRecordObserver(observer);
  provider->dispatch = [](Kind kind, const lepus::Value&) {
    return kind == Kind::kCallback ? Patch("args", Array(lepus::Value(88)))
                                   : shell::InterceptResult{};
  };
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  Deliver(native->saved_callbacks.at(0), 4);
  delegate->Drain();
  EXPECT_EQ(values, (std::vector<double>{88}));
  ASSERT_EQ(observer->data->callbacks.size(), 1u);
  EXPECT_EQ(observer->data->callbacks[0].second.GetProperty(0).Number(), 88);
  EXPECT_EQ(events.back(), "legacy.callback");
}
TEST_F(HostScriptModuleInterceptorTest, FailedHandlerDoesNotChangeExecution) {
  provider->dispatch = [](Kind, const lepus::Value&) {
    auto result = Patch("value", lepus::Value(999));
    result.patch.SetProperty("args", Array(lepus::Value(999)));
    result.mock = lepus::Value(lepus::Dictionary::Create());
    result.failed = true;
    return result;
  };
  Value args[] = {Value(3)};
  auto result = Call(args, 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 42);
  EXPECT_EQ(native->arguments.GetProperty(0).Number(), 3);
}
TEST_F(HostScriptModuleInterceptorTest,
       OpaqueAndCallbackArgumentsAreNotReplaced) {
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    if (kind != Kind::kCall) return shell::InterceptResult{};
    EXPECT_EQ(event.GetProperty("callbackIndices").GetProperty(0).Number(), 0);
    EXPECT_EQ(event.GetProperty("opaqueIndices").GetProperty(0).Number(), 1);
    auto replacements = lepus::CArray::Create();
    replacements->push_back(lepus::Value(1));
    replacements->push_back(lepus::Value(2));
    return Patch("args", lepus::Value(replacements));
  };
  std::vector<double> values;
  Object opaque(rt);
  opaque.setProperty(rt, "then", Value(true));
  Value args[] = {Callback(values), Value(std::move(opaque))};
  auto invocation = group->CreateInvocationGroup("Example", "echo", kView);
  std::vector<Value> replaced;
  invocation->RewriteArguments(rt, args, 2, replaced);
  ASSERT_EQ(replaced.size(), 2u);
  EXPECT_TRUE(Value::strictEquals(rt, args[0], replaced[0]));
  EXPECT_TRUE(Value::strictEquals(rt, args[1], replaced[1]));
}
TEST_F(HostScriptModuleInterceptorTest,
       DetachDoesNotBindOldCallbacksToNewProvider) {
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  auto callback = native->saved_callbacks.at(0);
  shell::Interceptor::Detach(provider.get());
  auto replacement = std::make_shared<Provider>();
  ASSERT_TRUE(shell::Interceptor::Attach(replacement,
                                         shell::Interceptor::Domain::kBTS));
  Deliver(callback, 6);
  delegate->Drain();
  EXPECT_EQ(values, (std::vector<double>{6}));
  EXPECT_TRUE(replacement->events.empty());
  shell::Interceptor::Detach(replacement.get());
}
TEST_F(HostScriptModuleInterceptorTest,
       DestroyedViewStopsCallbackInterception) {
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  auto count = provider->events.size();
  shell::Interceptor::DestroyView(kView);
  Deliver(native->saved_callbacks.at(0), 8);
  delegate->Drain();
  EXPECT_EQ(values, (std::vector<double>{8}));
  EXPECT_EQ(provider->events.size(), count);
}
TEST_F(HostScriptModuleInterceptorTest,
       NestedCallsAndImplicitCallbacksKeepTheirIdentity) {
  std::vector<double> values;
  auto id = delegate->RegisterJSCallbackFunction(
      Callback(values).getObject(rt).getFunction(rt));
  std::shared_ptr<ModuleCallback> outer, inner, restored;
  int depth = 0;
  native->invoke = [&] {
    if (depth++ == 0) {
      outer = std::make_shared<ModuleCallback>(id);
      EXPECT_TRUE(Call(nullptr, 0, "inner").has_value());
      restored = std::make_shared<ModuleCallback>(id);
    } else {
      inner = std::make_shared<ModuleCallback>(id);
    }
    --depth;
  };
  ASSERT_TRUE(Call(nullptr, 0, "outer").has_value());
  outer->timing_collector_ = std::make_shared<NativeModuleInfoCollector>(
      delegate, "Example", "outer", "", rt.GetPageUrl());
  Deliver(outer, 1);
  Deliver(inner, 2);
  Deliver(restored, 3);
  delegate->Drain();
  std::vector<std::string> ids, methods;
  for (auto& event : provider->events)
    if (event.first == Kind::kCallback) {
      ids.push_back(event.second.GetProperty("invocationId").StdString());
      methods.push_back(event.second.GetProperty("method").StdString());
      EXPECT_EQ(event.second.GetProperty("argumentIndex").Number(), -1);
    }
  ASSERT_EQ(ids.size(), 3u);
  EXPECT_EQ(ids[0], ids[2]);
  EXPECT_NE(ids[0], ids[1]);
  EXPECT_EQ(methods, (std::vector<std::string>{"outer", "inner", "outer"}));
  for (auto& event : events) EXPECT_NE(event, "legacy.callback");
}

TEST_F(HostScriptModuleInterceptorTest, LegacyHandledResultCanBeRewritten) {
  legacy->handled = true;
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    if (kind != Kind::kResult) return shell::InterceptResult{};
    EXPECT_EQ(event.GetProperty("value").Number(), 17);
    return Patch("value", lepus::Value(31));
  };
  auto result = Call();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 31);
  EXPECT_EQ(native->calls, 0);
}
TEST_F(HostScriptModuleInterceptorTest,
       NetworkConvertedCallbackIsRewrittenBeforeDelivery) {
  legacy->handled = true;
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    if (kind != Kind::kCallback) return shell::InterceptResult{};
    EXPECT_EQ(event.GetProperty("args").GetProperty(0).Number(), 12);
    return Patch("args", Array(lepus::Value(24)));
  };
  legacy->on_call = [&](const CallbackMap& callbacks) {
    auto callback = std::static_pointer_cast<ModuleCallback>(callbacks.at(0));
    callback->SetCustomArgsConverter([](Runtime*, ModuleCallback*) {
      return std::make_unique<pub::ValueImplLepus>(Array(lepus::Value(12)));
    });
    module->InvokeCallback(callback);
  };
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  delegate->Drain();
  EXPECT_EQ(values, (std::vector<double>{24}));
  EXPECT_EQ(events, (std::vector<std::string>{"legacy.before", "legacy.call",
                                              "legacy.callback"}));
}
TEST_F(HostScriptModuleInterceptorTest,
       NestedIdleCallDoesNotInheritOuterInvocation) {
  std::vector<double> values;
  auto id = delegate->RegisterJSCallbackFunction(
      Callback(values).getObject(rt).getFunction(rt));
  std::shared_ptr<ModuleCallback> outer, inner;
  int depth = 0;
  native->invoke = [&] {
    if (depth++ == 0) {
      outer = std::make_shared<ModuleCallback>(id);
      provider->enabled = false;
      EXPECT_TRUE(Call(nullptr, 0, "idle").has_value());
      provider->enabled = true;
    } else {
      inner = std::make_shared<ModuleCallback>(id);
    }
    --depth;
  };
  ASSERT_TRUE(Call(nullptr, 0, "outer").has_value());
  Deliver(inner, 1);
  Deliver(outer, 2);
  delegate->Drain();
  int callbacks = 0;
  for (const auto& event : provider->events)
    if (event.first == Kind::kCallback) {
      ++callbacks;
      EXPECT_EQ(event.second.GetProperty("method").StdString(), "outer");
    }
  EXPECT_EQ(callbacks, 1);
  EXPECT_EQ(values, (std::vector<double>{1, 2}));
}
#endif  // ENABLE_INSPECTOR
}  // namespace
}  // namespace lynx::runtime::js
