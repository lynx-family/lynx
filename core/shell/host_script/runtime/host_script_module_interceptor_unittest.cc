// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/host_script/runtime/host_script_module_interceptor.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/inspector/observer/native_module_record_observer.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module.h"
#include "core/runtime/js/bindings/modules/module_interceptor_registry.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime.h"
#include "core/shell/host_script/runtime/host_script_value_snapshot.h"
#include "core/shell/host_script/runtime/interceptor.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#if ENABLE_TESTBENCH_RECORDER
#include "base/include/fml/synchronization/waitable_event.h"
#define private public
#include "core/services/recorder/testbench_base_recorder.h"
#undef private
#endif

namespace lynx::shell {
namespace {
using runtime::CallbackMap;
using runtime::LynxModuleCallback;
using runtime::LynxNativeModule;
using runtime::js::Function;
using runtime::js::JSINativeException;
using runtime::js::LynxJSIModule;
using runtime::js::LynxModule;
using runtime::js::ModuleCallback;
using runtime::js::ModuleCallbackFunctionHolder;
using runtime::js::ModuleDelegate;
using runtime::js::ModuleInterceptor;
using runtime::js::ModuleInterceptorResult;
using runtime::js::NativeModuleInfo;
using runtime::js::NativeModuleInfoCollectorPtr;
using runtime::js::NativeModuleInvocationContext;
using runtime::js::NativeModuleRecordObserver;
using runtime::js::Object;
using runtime::js::PropNameID;
using runtime::js::QuickjsRuntime;
using runtime::js::Runtime;
using runtime::js::StringBuffer;
using runtime::js::Value;
using Kind = InterceptKind;

lepus::Value Array(lepus::Value value) {
  auto array = lepus::CArray::Create();
  array->push_back(std::move(value));
  return lepus::Value(array);
}
InterceptResult Patch(const char* key, lepus::Value value) {
  InterceptResult result;
  result.patch = lepus::Value(lepus::Dictionary::Create());
  result.patch.SetProperty(key, std::move(value));
  return result;
}
lepus::Value MockCallback(int index, lepus::Value args) {
  auto callback = lepus::Value(lepus::Dictionary::Create());
  callback.SetProperty("argumentIndex", lepus::Value(index));
  callback.SetProperty("args", std::move(args));
  return callback;
}

class Provider : public Interceptor {
 public:
  bool HasHandlers(Kind kind) const override {
    return enabled && ((kind == Kind::kCall && call_enabled) ||
                       (kind == Kind::kResult && result_enabled) ||
                       (kind == Kind::kCallback && callback_enabled));
  }
  InterceptResult Dispatch(Kind kind, const lepus::Value& event) override {
    events.emplace_back(kind, event);
    return dispatch ? dispatch(kind, event) : InterceptResult{};
  }
  void ReportError(const std::string&) override {}
  std::vector<lepus::Value> Events(Kind kind) const {
    std::vector<lepus::Value> matched;
    for (const auto& [event_kind, event] : events) {
      if (event_kind == kind) matched.push_back(event);
    }
    return matched;
  }
  bool enabled = true;
  bool call_enabled = true;
  bool result_enabled = true;
  bool callback_enabled = true;
  std::function<InterceptResult(Kind, const lepus::Value&)> dispatch;
  std::vector<std::pair<Kind, lepus::Value>> events;
};

// Model platform pub::Value containers without requiring a platform runtime.
class CallbackValue : public pub::ValueImplLepus {
 public:
  enum class Type {
    kArray,
    kMap,
    kTransfer,
    kLynxObject,
    kTemplateData,
    kUnknown
  };
  explicit CallbackValue(Type type, std::unique_ptr<pub::Value> child = nullptr)
      : ValueImplLepus(lepus::Value()), type_(type), child_(std::move(child)) {
    backend_type_ = pub::ValueBackendType::ValueBackendTypeCustom;
  }
  bool IsNil() const override { return false; }
  bool IsArray() const override { return type_ == Type::kArray; }
  bool IsMap() const override { return type_ == Type::kMap; }
  bool IsTransfer() const override { return type_ == Type::kTransfer; }
  bool IsLynxObject() const override { return type_ == Type::kLynxObject; }
  bool IsTemplateData() const override { return type_ == Type::kTemplateData; }
  int Length() const override { return 1; }
  void ForeachArray(pub::ForeachArrayFunc func) const override {
    if (IsArray()) {
      func(0, child_ ? *child_ : static_cast<const pub::Value&>(*this));
    }
  }
  void ForeachMap(pub::ForeachMapFunc func) const override {
    if (IsMap()) func(pub::ValueImplLepus("payload"), *child_);
  }

 private:
  Type type_;
  std::unique_ptr<pub::Value> child_;
};

// Exercises the legacy hooks and short-circuit contract of RequestInterceptor.
class LegacyInterceptor : public ModuleInterceptor {
 public:
  explicit LegacyInterceptor(std::vector<std::string>& events)
      : events(events) {}
  ModuleInterceptorResult InterceptModuleMethod(
      const std::shared_ptr<LynxModule>&, const LynxModule::MethodMetadata&,
      Runtime*, const std::shared_ptr<ModuleDelegate>&, const Value*, size_t,
      const std::unique_ptr<pub::Value>&, const CallbackMap& callbacks,
      NativeModuleInfoCollectorPtr,
      runtime::js::ModuleCallContext* = nullptr) const override {
    events.push_back("legacy.call");
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
  bool handled = false;
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
    return std::unique_ptr<pub::Value>(new pub::ValueImplLepus(result));
  }
  lepus::Value result = lepus::Value(42);
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
    auto id = ++next_id;
    functions.emplace(id, std::make_unique<ModuleCallbackFunctionHolder>(
                              std::move(function)));
    return id;
  }
  void CallJSCallback(const std::shared_ptr<ModuleCallback>& callback,
                      base::MoveOnlyClosure<bool> pre = nullptr,
                      int64_t id_to_delete = ModuleCallback::kInvalidCallbackId,
                      bool force_async = false) override {
    if (force_async) {
      EXPECT_FALSE(pre);
      EXPECT_EQ(id_to_delete, ModuleCallback::kInvalidCallbackId);
      pending.push_back(callback);
      return;
    }
    if (pre && !pre()) return;
    if (id_to_delete != ModuleCallback::kInvalidCallbackId) {
      ++releases[id_to_delete];
      EXPECT_EQ(functions.erase(id_to_delete), 1u);
    }
    if (!callback) return;
    auto entry = functions.find(callback->callback_id());
    ASSERT_NE(entry, functions.end());
    auto holder = std::move(entry->second);
    functions.erase(entry);
    callback->Invoke(&rt, holder.get());
  }
  void Drain() {
    auto deliveries = std::move(pending);
    pending.clear();
    for (auto& callback : deliveries) {
      CallJSCallback(callback);
    }
  }
  Runtime& rt;
  int64_t next_id = 0;
  std::unordered_map<int64_t, std::unique_ptr<ModuleCallbackFunctionHolder>>
      functions;
  std::vector<std::shared_ptr<ModuleCallback>> pending;
  std::unordered_map<int64_t, int> releases;
};

struct Recorded {
  lepus::Value args;
  std::optional<lepus::Value> result;
  bool success = false;
  int invokes = 0;
  std::vector<std::pair<int, lepus::Value>> callbacks;
  std::vector<std::string> order;
  std::unordered_map<int, int64_t> registered_callbacks;
};
class RecordContext : public NativeModuleInvocationContext {
 public:
  explicit RecordContext(std::shared_ptr<Recorded> data, int index = -1)
      : data(data), index(index) {}
  std::shared_ptr<NativeModuleInvocationContext> WithCallbackArgumentIndex(
      int32_t index) const override {
    return std::make_shared<RecordContext>(data, index);
  }
  void OnInvoke(lepus::Value args, const CallbackMap& callbacks, bool success,
                std::optional<lepus::Value> result, int32_t,
                const std::string&) const override {
    ++data->invokes;
    data->order.push_back("invoke");
    data->args = args;
    data->success = success;
    data->result = result;
    data->registered_callbacks.clear();
    for (const auto& [index, callback] : callbacks) {
      data->registered_callbacks.emplace(index, callback->CallbackId());
    }
  }
  void OnCallback(lepus::Value value) const override {
    data->order.push_back("callback");
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
    ASSERT_TRUE(Interceptor::Attach(provider, Interceptor::Domain::kBTS));
    Interceptor::RegisterView(kView);
    // Same order as ModuleInterceptorRegistry::Install: Host Script leads.
    group->AddLeadingInterceptor(
        std::make_unique<HostScriptModuleInterceptor>());
    auto legacy_ptr = std::make_unique<LegacyInterceptor>(events);
    legacy = legacy_ptr.get();
    group->AddInterceptor(std::move(legacy_ptr));
    module->SetLogContext({kView, 0, 0});
    module->SetModuleInterceptor(group);
    module->SetNativeModuleRecordObserver(observer);
    native->SetDelegate(module);
    native->invoke = [&] { events.push_back("native"); };
  }
  void TearDown() override {
    delegate->pending.clear();
    Interceptor::DestroyView(kView);
    Interceptor::Detach(provider.get());
  }
  base::expected<Value, JSINativeException> Call(
      const Value* args = nullptr, size_t count = 0,
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
  Value Evaluate(const char* source) {
    auto value = rt.evaluateJavaScript(std::make_shared<StringBuffer>(source),
                                       "test.js");
    EXPECT_TRUE(value.has_value());
    return value.has_value() ? std::move(*value) : Value::undefined();
  }
  void MockCall(std::vector<lepus::Value> callbacks,
                std::optional<lepus::Value> return_value = std::nullopt) {
    provider->dispatch = [callbacks, return_value](Kind kind,
                                                   const lepus::Value&) {
      InterceptResult decision;
      if (kind == Kind::kResult) return Patch("value", lepus::Value(99));
      if (kind == Kind::kCallback)
        return Patch("args", Array(lepus::Value(88)));
      decision.mock = lepus::Value(lepus::Dictionary::Create());
      if (return_value) decision.mock.SetProperty("returnValue", *return_value);
      auto deliveries = lepus::CArray::Create();
      for (const auto& callback : callbacks) deliveries->push_back(callback);
      decision.mock.SetProperty("callbacks", lepus::Value(deliveries));
      return decision;
    };
  }

  static constexpr base::LynxEntityId kView = 42;
  QuickjsRuntime rt;
  std::vector<std::string> events;
  std::shared_ptr<Provider> provider = std::make_shared<Provider>();
  std::shared_ptr<Delegate> delegate = std::make_shared<Delegate>(rt);
  std::shared_ptr<NativeModule> native = std::make_shared<NativeModule>();
  std::shared_ptr<LynxJSIModule> module =
      std::make_shared<LynxJSIModule>("Example", delegate, native);
  std::shared_ptr<runtime::js::GroupInterceptor> group =
      std::make_shared<runtime::js::GroupInterceptor>();
  std::shared_ptr<Observer> observer = std::make_shared<Observer>();
  LegacyInterceptor* legacy = nullptr;
};

// Invocation stages.

TEST_F(HostScriptModuleInterceptorTest, IdleProviderLeavesCallUntouched) {
  provider->enabled = false;
  EXPECT_TRUE(Call().has_value());
  EXPECT_TRUE(provider->events.empty());
  EXPECT_EQ(events, (std::vector<std::string>{"legacy.before", "legacy.call",
                                              "native"}));
}

TEST_F(HostScriptModuleInterceptorTest, NoProviderUsesOriginalChain) {
  Interceptor::Detach(provider.get());
  ASSERT_TRUE(Call().has_value());
  EXPECT_EQ(native->calls, 1);
  EXPECT_TRUE(provider->events.empty());
}

TEST_F(HostScriptModuleInterceptorTest, ResultOnlyHandlerGetsCallContext) {
  provider->call_enabled = false;
  provider->callback_enabled = false;
  provider->dispatch = [](Kind kind, const lepus::Value&) {
    EXPECT_EQ(kind, Kind::kResult);
    return Patch("value", lepus::Value(9));
  };
  auto result = Call();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 9);
  EXPECT_TRUE(provider->Events(Kind::kCall).empty());
  ASSERT_EQ(provider->Events(Kind::kResult).size(), 1u);
  EXPECT_FALSE(provider->Events(Kind::kResult)[0]
                   .GetProperty("invocationId")
                   .StdString()
                   .empty());
}

TEST_F(HostScriptModuleInterceptorTest, CallbackOnlyHandlerKeepsCallContext) {
  provider->call_enabled = false;
  provider->result_enabled = false;
  provider->dispatch = [](Kind kind, const lepus::Value&) {
    EXPECT_EQ(kind, Kind::kCallback);
    return Patch("args", Array(lepus::Value(9)));
  };
  std::vector<double> values;
  Value callback = Callback(values);
  ASSERT_TRUE(Call(&callback, 1, "original").has_value());
  auto saved = native->saved_callbacks.at(0);
  ASSERT_TRUE(Call(nullptr, 0, "later").has_value());
  EXPECT_TRUE(provider->events.empty());
  Deliver(saved, 7);
  EXPECT_EQ(values, std::vector<double>{9});
  ASSERT_EQ(provider->events.size(), 1u);
  EXPECT_EQ(provider->events[0].second.GetProperty("method").StdString(),
            "original");
}

TEST_F(HostScriptModuleInterceptorTest, UnregisteredViewReportsCoverageGap) {
  Interceptor::DestroyView(kView);
  const auto before = Interceptor::FailureCount();
  ASSERT_TRUE(Call().has_value());
  EXPECT_EQ(Interceptor::FailureCount(), before + 1);
  EXPECT_TRUE(provider->events.empty());
  EXPECT_EQ(native->calls, 1);
}

TEST_F(HostScriptModuleInterceptorTest, MissingProviderReportsOneGap) {
  const auto before = Interceptor::FailureCount();
  for (auto kind : {Kind::kCall, Kind::kResult, Kind::kCallback}) {
    Interceptor::HandlerAdded(kind, Interceptor::Domain::kBTS);
  }
  // Providers are thread-affine; another thread has none attached.
  std::thread other([this] {
    CallbackMap callbacks;
    LynxModule::MethodMetadata method(0, "echo");
    std::vector<Value> rewritten;
    HostScriptModuleInterceptor interceptor;
    auto context = interceptor.BeforeConvertArgs(module, method, &rt, nullptr,
                                                 0, rewritten);
    ASSERT_NE(context, nullptr);
    Value result(42);
    EXPECT_FALSE(interceptor.AfterInvokeMethod(&rt, context.get(), result));
    EXPECT_EQ(result.getNumber(), 42);
  });
  other.join();
  EXPECT_EQ(Interceptor::FailureCount(), before + 1);
  for (auto kind : {Kind::kCall, Kind::kResult, Kind::kCallback}) {
    Interceptor::HandlerRemoved(kind, Interceptor::Domain::kBTS);
  }
}

TEST_F(HostScriptModuleInterceptorTest, RegistrationInstallsOnce) {
  HostScriptModuleInterceptor::Register();
  HostScriptModuleInterceptor::Register();
  // What LynxModuleManager::InitModuleInterceptor installs without built-ins.
  auto installed = runtime::js::ModuleInterceptorRegistry::Install(nullptr);
  ASSERT_NE(installed, nullptr);
  module->SetModuleInterceptor(installed);
  ASSERT_TRUE(Call().has_value());
  // One call stage and one result stage.
  EXPECT_EQ(provider->events.size(), 2u);
}

TEST_F(HostScriptModuleInterceptorTest,
       RewritesBeforeConversionAndRecordsEffectiveResult) {
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
  EXPECT_EQ(provider->events[0].second.GetProperty("viewId").StdString(), "42");
  EXPECT_EQ(provider->events[0].second.GetProperty("module").StdString(),
            "Example");
}

TEST_F(HostScriptModuleInterceptorTest, OpaqueAndCallbackArgumentsAreKept) {
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    if (kind != Kind::kCall) return InterceptResult{};
    EXPECT_EQ(event.GetProperty("callbackIndices").GetProperty(0).Number(), 0);
    EXPECT_EQ(event.GetProperty("opaqueIndices").GetProperty(0).Number(), 1);
    EXPECT_EQ(event.GetProperty("args")
                  .GetProperty(0)
                  .GetProperty("callback")
                  .Number(),
              0);
    auto replacements = lepus::CArray::Create();
    replacements->push_back(lepus::Value(1));
    replacements->push_back(lepus::Value(2));
    replacements->push_back(lepus::Value(3));
    return Patch("args", lepus::Value(replacements));
  };
  std::vector<double> values;
  Object opaque(rt);
  opaque.setProperty(rt, "then", Value(true));
  Value args[] = {Callback(values), Value(std::move(opaque)), Value(0)};
  ASSERT_TRUE(Call(args, 3).has_value());
  EXPECT_EQ(native->saved_callbacks.size(), 1u);
  EXPECT_TRUE(native->arguments.GetProperty(1).GetProperty("then").Bool());
  EXPECT_EQ(native->arguments.GetProperty(2).Number(), 3);
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

TEST_F(HostScriptModuleInterceptorTest, NativeFailureKeepsErrorAndRecording) {
  native->fail = true;
  EXPECT_FALSE(Call().has_value());
  EXPECT_EQ(observer->data->invokes, 1);
  EXPECT_FALSE(observer->data->success);
  EXPECT_FALSE(observer->data->result.has_value());
  EXPECT_TRUE(provider->Events(Kind::kResult).empty());
}

TEST_F(HostScriptModuleInterceptorTest, NoResultPatchKeepsNativeRecording) {
  // A JS Number would be recorded as a double if the native snapshot were
  // needlessly converted again after an empty result patch.
  constexpr int64_t exact = 42;
  native->result = lepus::Value(exact);
  ASSERT_TRUE(Call().has_value());
  ASSERT_TRUE(observer->data->result);
  EXPECT_TRUE(observer->data->result->IsInt64());
  EXPECT_EQ(observer->data->result->Int64(), exact);
  EXPECT_EQ(provider->events.back().first, Kind::kResult);
}

TEST_F(HostScriptModuleInterceptorTest, UndefinedResultClearsNativeRecording) {
  provider->dispatch = [](Kind kind, const lepus::Value&) {
    if (kind != Kind::kResult) return InterceptResult{};
    lepus::Value value;
    value.SetUndefined();
    return Patch("value", value);
  };
  auto result = Call();
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->isUndefined());
  EXPECT_FALSE(observer->data->result.has_value());
}

TEST_F(HostScriptModuleInterceptorTest, LegacyHandledResultCanBeRewritten) {
  legacy->handled = true;
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    if (kind != Kind::kResult) return InterceptResult{};
    EXPECT_EQ(event.GetProperty("value").Number(), 17);
    return Patch("value", lepus::Value(31));
  };
  auto result = Call();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 31);
  EXPECT_EQ(native->calls, 0);
}

TEST_F(HostScriptModuleInterceptorTest, ArgumentPatchIsNotRetained) {
  auto payload = lepus::CArray::Create();
  payload->push_back(lepus::Value(9));
  provider->dispatch = [payload](Kind kind, const lepus::Value&) {
    return kind == Kind::kCall ? Patch("args", lepus::Value(payload))
                               : InterceptResult{};
  };
  std::vector<double> values;
  Value args[] = {Value(1), Callback(values)};
  ASSERT_TRUE(Call(args, 2).has_value());
  provider->dispatch = nullptr;
  // The callback still shares the invocation state.
  ASSERT_EQ(native->saved_callbacks.size(), 1u);
  EXPECT_TRUE(payload->HasOneRef());
  EXPECT_EQ(native->arguments.GetProperty(0).Number(), 9);
}

// Mock decisions.

TEST_F(HostScriptModuleInterceptorTest,
       MockSkipsNativeAndDeliversDistinctCallbacks) {
  MockCall({MockCallback(2, Array(lepus::Value(20))),
            MockCallback(1, Array(lepus::Value(10)))},
           lepus::Value());
  std::vector<double> values;
  Value callback = Callback(values);
  Value args[] = {Value(7), Value(rt, callback), Value(rt, callback),
                  Callback(values)};
  auto result = Call(args, 4);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->isNull());
  EXPECT_TRUE(values.empty());
  EXPECT_EQ(observer->data->order, (std::vector<std::string>{"invoke"}));
  ASSERT_EQ(delegate->pending.size(), 2u);
  EXPECT_EQ(delegate->pending[0]->callback_id(), 2);
  EXPECT_EQ(delegate->pending[1]->callback_id(), 1);
  EXPECT_EQ(delegate->functions.size(), 2u);
  EXPECT_EQ(delegate->releases, (std::unordered_map<int64_t, int>{{3, 1}}));
  EXPECT_EQ(observer->data->registered_callbacks,
            (std::unordered_map<int, int64_t>{{1, 1}, {2, 2}, {3, 3}}));
  delegate->Drain();
  EXPECT_EQ(values, (std::vector<double>{20, 10}));
  EXPECT_EQ(observer->data->order,
            (std::vector<std::string>{"invoke", "callback", "callback"}));
  EXPECT_TRUE(delegate->functions.empty());
  // Hooks before interception still run; the legacy interceptor is never
  // asked to handle the call nor told about mocked callbacks.
  EXPECT_EQ(events, (std::vector<std::string>{"legacy.before"}));
  EXPECT_EQ(native->calls, 0);
  ASSERT_TRUE(observer->data->result);
  EXPECT_TRUE(observer->data->result->IsNil());
  ASSERT_EQ(observer->data->callbacks.size(), 2u);
  EXPECT_EQ(observer->data->callbacks[0].first, 2);
  EXPECT_EQ(observer->data->callbacks[0].second.GetProperty(0).Number(), 20);
  EXPECT_EQ(observer->data->callbacks[1].first, 1);
  EXPECT_EQ(observer->data->callbacks[1].second.GetProperty(0).Number(), 10);
}

TEST_F(HostScriptModuleInterceptorTest, MockSkipsResultAndCallbackStages) {
  MockCall({MockCallback(0, Array(lepus::Value(11)))}, lepus::Value(7));
  std::vector<double> values;
  Value args[] = {Callback(values)};
  auto result = Call(args, 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 7);
  ASSERT_TRUE(observer->data->result);
  EXPECT_EQ(observer->data->result->Number(), 7);
  delegate->Drain();
  EXPECT_EQ(values, (std::vector<double>{11}));
  ASSERT_EQ(observer->data->callbacks.size(), 1u);
  EXPECT_EQ(observer->data->callbacks[0].second.GetProperty(0).Number(), 11);
  ASSERT_EQ(provider->events.size(), 1u);
  EXPECT_EQ(provider->events[0].first, Kind::kCall);
  EXPECT_TRUE(delegate->functions.empty());
  EXPECT_TRUE(delegate->releases.empty());
}

TEST_F(HostScriptModuleInterceptorTest, MockDeliversEachArgumentAtMostOnce) {
  // Providers normally reject duplicate indices. Also guard delivery when a
  // native provider supplies an unchecked decision.
  MockCall({MockCallback(0, Array(lepus::Value(10))),
            MockCallback(0, Array(lepus::Value(20)))});
  std::vector<double> values;
  Value args[] = {Callback(values), Callback(values)};
  ASSERT_TRUE(Call(args, 2).has_value());
  ASSERT_EQ(delegate->pending.size(), 1u);
  EXPECT_EQ(delegate->releases, (std::unordered_map<int64_t, int>{{2, 1}}));
  delegate->Drain();
  EXPECT_EQ(values, (std::vector<double>{10}));
  EXPECT_TRUE(delegate->functions.empty());
}

TEST_F(HostScriptModuleInterceptorTest, MockReleasesUndeliveredCallbacks) {
  MockCall({});
  std::vector<double> values;
  Value args[] = {Callback(values), Callback(values)};
  ASSERT_TRUE(Call(args, 2).has_value());
  delegate->Drain();
  EXPECT_TRUE(delegate->functions.empty());
  EXPECT_TRUE(values.empty());
  EXPECT_EQ(delegate->releases,
            (std::unordered_map<int64_t, int>{{1, 1}, {2, 1}}));
  EXPECT_EQ(native->calls, 0);
}

TEST_F(HostScriptModuleInterceptorTest, MockRecordsNullButNotMissingReturn) {
  MockCall({});
  auto without_return = Call(nullptr, 0, "withoutReturn");
  ASSERT_TRUE(without_return.has_value());
  EXPECT_TRUE(without_return->isUndefined());
  EXPECT_FALSE(observer->data->result);
  MockCall({}, lepus::Value());
  auto with_return = Call(nullptr, 0, "withReturn");
  ASSERT_TRUE(with_return.has_value());
  EXPECT_TRUE(with_return->isNull());
  ASSERT_TRUE(observer->data->result);
  EXPECT_TRUE(observer->data->result->IsNil());
  ASSERT_EQ(provider->events.size(), 2u);
  for (const auto& event : provider->events) {
    EXPECT_EQ(event.first, Kind::kCall);
  }
}

TEST_F(HostScriptModuleInterceptorTest, MockWithoutRecordingObserver) {
  module->SetNativeModuleRecordObserver({});
  MockCall({MockCallback(0, Array(lepus::Value(5)))});
  std::vector<double> values;
  Value args[] = {Callback(values)};
  auto result = Call(args, 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->isUndefined());
  delegate->Drain();
  EXPECT_EQ(values, (std::vector<double>{5}));
}

TEST_F(HostScriptModuleInterceptorTest, MockDecisionIsNotRetained) {
  auto payload = lepus::Dictionary::Create();
  provider->dispatch = [payload](Kind kind, const lepus::Value&) {
    InterceptResult result;
    if (kind == Kind::kCall) result.mock = lepus::Value(payload);
    return result;
  };
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  provider->dispatch = nullptr;
  EXPECT_TRUE(payload->HasOneRef());
}

TEST_F(HostScriptModuleInterceptorTest,
       MockConversionFailureReleasesRegisteredCallbacks) {
  MockCall({MockCallback(0, Array(lepus::Value(10)))});
  std::vector<double> values;
  Object cyclic(rt);
  cyclic.setProperty(rt, "self", Value(rt, cyclic));
  auto array = runtime::js::Array::createWithLength(rt, 1);
  ASSERT_TRUE(array);
  ASSERT_TRUE(array->setValueAtIndex(rt, 0, Value(rt, *array)));
  for (auto* failing : {&cyclic, static_cast<Object*>(&*array)}) {
    delegate->releases.clear();
    Value args[] = {Callback(values), Callback(values), Value(rt, *failing)};
    EXPECT_FALSE(Call(args, 3).has_value());
    EXPECT_TRUE(delegate->pending.empty());
    EXPECT_TRUE(delegate->functions.empty());
    EXPECT_EQ(delegate->releases.size(), 2u);
  }
  EXPECT_TRUE(values.empty());
  EXPECT_EQ(native->calls, 0);
}

TEST_F(HostScriptModuleInterceptorTest, MockCallbackReentersIndependently) {
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    InterceptResult result;
    if (kind == Kind::kCall &&
        event.GetProperty("method").StdString() == "outer") {
      result.mock = lepus::Value(lepus::Dictionary::Create());
      result.mock.SetProperty("callbacks",
                              Array(MockCallback(0, Array(lepus::Value(1)))));
    }
    return result;
  };
  bool returned = false;
  bool invoked = false;
  Value callback(Function::createFromHostFunction(
      rt, PropNameID::forAscii(rt, "cb"), 1,
      [&](Runtime&, const Value&, const Value*, size_t) {
        EXPECT_TRUE(returned);
        EXPECT_TRUE(Call(nullptr, 0, "nested").has_value());
        invoked = true;
        return Value::undefined();
      }));
  ASSERT_TRUE(Call(&callback, 1, "outer").has_value());
  returned = true;
  EXPECT_FALSE(invoked);
  delegate->Drain();
  EXPECT_TRUE(invoked);
  ASSERT_EQ(provider->events.size(), 3u);
  EXPECT_EQ(provider->events[1].second.GetProperty("method").StdString(),
            "nested");
  EXPECT_EQ(provider->events[2].first, Kind::kResult);
  EXPECT_EQ(provider->events[2].second.GetProperty("invocationId"),
            provider->events[1].second.GetProperty("invocationId"));
  EXPECT_NE(provider->events[1].second.GetProperty("invocationId"),
            provider->events[0].second.GetProperty("invocationId"));
  EXPECT_TRUE(delegate->functions.empty());
}

// Callbacks.

TEST_F(HostScriptModuleInterceptorTest, CallbackRewriteBeforeJSAndRecording) {
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    if (kind != Kind::kCallback) return InterceptResult{};
    EXPECT_EQ(event.GetProperty("argumentIndex").Number(), 0);
    return Patch("args", Array(lepus::Value(88)));
  };
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  Deliver(native->saved_callbacks.at(0), 4);
  EXPECT_EQ(values, (std::vector<double>{88}));
  ASSERT_EQ(observer->data->callbacks.size(), 1u);
  EXPECT_EQ(observer->data->callbacks[0].second.GetProperty(0).Number(), 88);
  EXPECT_EQ(events.back(), "legacy.callback");
}

TEST_F(HostScriptModuleInterceptorTest, NativeCallbacksRemainInline) {
  native->invoke = [&] { Deliver(native->saved_callbacks.at(0), 7); };
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  EXPECT_EQ(values, (std::vector<double>{7}));
  EXPECT_TRUE(delegate->pending.empty());
  EXPECT_TRUE(delegate->releases.empty());
  auto callbacks = provider->Events(Kind::kCallback);
  ASSERT_EQ(callbacks.size(), 1u);
  EXPECT_FALSE(callbacks[0].Contains("callbackKind"));
}

TEST_F(HostScriptModuleInterceptorTest, NetworkConvertedCallbackIsRewritten) {
  legacy->handled = true;
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    if (kind != Kind::kCallback) return InterceptResult{};
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
  EXPECT_EQ(values, (std::vector<double>{24}));
  EXPECT_EQ(events, (std::vector<std::string>{"legacy.before", "legacy.call",
                                              "legacy.callback"}));
}

TEST_F(HostScriptModuleInterceptorTest, InlineHandledCallbackStopsAtHandler) {
  legacy->handled = true;
  legacy->on_call = [&](const CallbackMap& callbacks) {
    Deliver(callbacks.at(0), 7);
  };
  group->AddInterceptor(std::make_unique<LegacyInterceptor>(events));
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  EXPECT_EQ(values, (std::vector<double>{7}));
  EXPECT_EQ(std::count(events.begin(), events.end(), "legacy.callback"), 1);
  EXPECT_EQ(provider->Events(Kind::kCallback).size(), 1u);
}

TEST_F(HostScriptModuleInterceptorTest, RecordingWorksWithoutHostScript) {
  Interceptor::Detach(provider.get());
  std::vector<double> values;
  Value args[] = {Callback(values)};
  auto result = Call(args, 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 42);
  Deliver(native->saved_callbacks.at(0), 7);
  EXPECT_TRUE(provider->events.empty());
  ASSERT_TRUE(observer->data->result);
  EXPECT_EQ(observer->data->result->Number(), 42);
  ASSERT_EQ(observer->data->callbacks.size(), 1u);
  EXPECT_EQ(observer->data->callbacks[0].second.GetProperty(0).Number(), 7);
}

TEST_F(HostScriptModuleInterceptorTest, DetachedProviderIsNotReplaced) {
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  auto callback = native->saved_callbacks.at(0);
  Interceptor::Detach(provider.get());
  auto replacement = std::make_shared<Provider>();
  ASSERT_TRUE(Interceptor::Attach(replacement, Interceptor::Domain::kBTS));
  Deliver(callback, 6);
  EXPECT_EQ(values, (std::vector<double>{6}));
  EXPECT_TRUE(replacement->events.empty());
  Interceptor::Detach(replacement.get());
}

TEST_F(HostScriptModuleInterceptorTest, DestroyedViewStopsCallbacks) {
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  auto count = provider->events.size();
  Interceptor::DestroyView(kView);
  Deliver(native->saved_callbacks.at(0), 8);
  EXPECT_EQ(values, (std::vector<double>{8}));
  EXPECT_EQ(provider->events.size(), count);
}

TEST_F(HostScriptModuleInterceptorTest, NestedCallbacksKeepTheirInvocation) {
  std::vector<double> values;
  std::shared_ptr<LynxModuleCallback> outer, inner, restored;
  int depth = 0;
  native->invoke = [&] {
    if (depth++ == 0) {
      outer = native->saved_callbacks.at(0);
      restored = native->saved_callbacks.at(1);
      Value callback = Callback(values);
      EXPECT_TRUE(Call(&callback, 1, "inner").has_value());
    } else {
      inner = native->saved_callbacks.at(0);
    }
    --depth;
  };
  Value args[] = {Callback(values), Callback(values)};
  ASSERT_TRUE(Call(args, 2, "outer").has_value());
  Deliver(outer, 1);
  Deliver(inner, 2);
  Deliver(restored, 3);
  std::vector<std::string> ids, methods;
  std::vector<int> indices;
  for (const auto& event : provider->Events(Kind::kCallback)) {
    ids.push_back(event.GetProperty("invocationId").StdString());
    methods.push_back(event.GetProperty("method").StdString());
    indices.push_back(event.GetProperty("argumentIndex").Number());
  }
  ASSERT_EQ(ids.size(), 3u);
  EXPECT_EQ(ids[0], ids[2]);
  EXPECT_NE(ids[0], ids[1]);
  EXPECT_EQ(methods, (std::vector<std::string>{"outer", "inner", "outer"}));
  EXPECT_EQ(indices, (std::vector<int>{0, 0, 1}));
  EXPECT_EQ(values, (std::vector<double>{1, 2, 3}));
  EXPECT_TRUE(delegate->functions.empty());
}

TEST_F(HostScriptModuleInterceptorTest, NestedIdleCallIsNotIntercepted) {
  std::vector<double> values;
  std::shared_ptr<LynxModuleCallback> outer, inner;
  Value args[] = {Callback(values)};
  int depth = 0;
  native->invoke = [&] {
    if (depth++ == 0) {
      outer = native->saved_callbacks.at(0);
      provider->enabled = false;
      EXPECT_TRUE(Call(args, 1, "idle").has_value());
      provider->enabled = true;
    } else {
      inner = native->saved_callbacks.at(0);
    }
    --depth;
  };
  ASSERT_TRUE(Call(args, 1, "outer").has_value());
  Deliver(inner, 1);
  Deliver(outer, 2);
  auto callbacks = provider->Events(Kind::kCallback);
  ASSERT_EQ(callbacks.size(), 1u);
  EXPECT_EQ(callbacks[0].GetProperty("method").StdString(), "outer");
  EXPECT_EQ(values, (std::vector<double>{1, 2}));
}

TEST_F(HostScriptModuleInterceptorTest, DelayedCallbackKeepsItsInvocation) {
  std::vector<double> values;
  Value callback = Callback(values);
  ASSERT_TRUE(Call(&callback, 1, "original").has_value());
  auto original = native->saved_callbacks.at(0);
  std::shared_ptr<LynxModuleCallback> delayed;
  native->invoke = [&] {
    std::thread worker([&] { delayed = original; });
    worker.join();
  };
  ASSERT_TRUE(Call(nullptr, 0, "unrelated").has_value());
  Deliver(delayed, 8);
  EXPECT_EQ(values, (std::vector<double>{8}));
  ASSERT_EQ(provider->events.back().first, Kind::kCallback);
  EXPECT_EQ(provider->events.back().second.GetProperty("method").StdString(),
            "original");
}

TEST_F(HostScriptModuleInterceptorTest, UnrelatedCallbackIsNotIntercepted) {
  std::vector<double> values;
  std::shared_ptr<ModuleCallback> unrelated;
  native->invoke = [&] {
    auto id = delegate->RegisterJSCallbackFunction(
        Callback(values).getObject(rt).getFunction(rt));
    unrelated = std::make_shared<ModuleCallback>(id);
    unrelated->SetModuleInterceptor(group);
  };
  ASSERT_TRUE(Call().has_value());
  auto count = provider->events.size();
  Deliver(unrelated, 4);
  EXPECT_EQ(provider->events.size(), count);
  EXPECT_EQ(values, (std::vector<double>{4}));
}

TEST_F(HostScriptModuleInterceptorTest, PlatformCallbackValuesArePatched) {
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    if (kind != Kind::kCallback) return InterceptResult{};
    EXPECT_EQ(event.GetProperty("args")
                  .GetProperty(0)
                  .GetProperty("payload")
                  .Number(),
              42);
    return Patch("args", Array(lepus::Value(73)));
  };
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  using Type = CallbackValue::Type;
  auto callback = native->saved_callbacks.at(0);
  callback->SetArgs(std::make_unique<CallbackValue>(
      Type::kArray,
      std::make_unique<CallbackValue>(
          Type::kMap, std::make_unique<pub::ValueImplLepus>(42))));
  module->InvokeCallback(callback);
  EXPECT_EQ(values, (std::vector<double>{73}));
}

TEST_F(HostScriptModuleInterceptorTest, JavaScriptPromiseUsesExplicitCallback) {
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    if (kind != Kind::kCallback) return InterceptResult{};
    EXPECT_EQ(event.GetProperty("argumentIndex").Number(), 0);
    return Patch("args", Array(lepus::Value(73)));
  };
  for (bool reject : {false, true}) {
    auto promise = rt.global().getPropertyAsFunction(rt, "Promise");
    ASSERT_TRUE(promise);
    auto executor = Function::createFromHostFunction(
        rt, PropNameID::forAscii(rt, "executor"), 2,
        [&](Runtime&, const Value&, const Value* args, size_t count) {
          EXPECT_EQ(count, 2u);
          EXPECT_TRUE(
              Call(args + (reject ? 1 : 0), 1, "wrappedPromise").has_value());
          return Value::undefined();
        });
    auto response = promise->callAsConstructor(rt, executor);
    ASSERT_TRUE(response.has_value());
    rt.global().setProperty(rt, "testPromise", Value(rt, *response));
    Evaluate(
        "globalThis.fulfilled = null; globalThis.rejected = null;"
        "testPromise.then(v => { fulfilled = v; }, e => { rejected = e; });");
    Deliver(native->saved_callbacks.at(0), 1);
    LEPUSContext* context = nullptr;
    int status = 0;
    for (int i = 0; i < 100; ++i) {
      status = LEPUS_ExecutePendingJob(rt.getJSRuntime(), &context);
      if (status <= 0) break;
    }
    EXPECT_EQ(status, 0);
    auto settled =
        rt.global().getProperty(rt, reject ? "rejected" : "fulfilled");
    ASSERT_TRUE(settled && settled->isNumber());
    EXPECT_EQ(settled->getNumber(), 73);
  }
}

// Snapshots.

TEST_F(HostScriptModuleInterceptorTest, ObjectLengthDoesNotSizeSnapshot) {
  for (double length : {100000000.0, -1.0}) {
    Object object(rt);
    object.setProperty(rt, "length", Value(length));
    object.setProperty(rt, "answer", Value(42));
    Object outer(rt);
    outer.setProperty(rt, "payload", Value(rt, object));
    for (auto* value : {&object, &outer}) {
      auto snapshot = SnapshotJSValue(rt, Value(rt, *value));
      ASSERT_TRUE(snapshot);
      if (value == &outer) snapshot = snapshot->GetProperty("payload");
      EXPECT_EQ(snapshot->GetLength(), 2);
      EXPECT_EQ(snapshot->GetProperty("length").Number(), length);
      EXPECT_EQ(snapshot->GetProperty("answer").Number(), 42);
    }
  }
}

TEST_F(HostScriptModuleInterceptorTest, LossyValuesStayOpaque) {
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    return kind == Kind::kCall ? Patch("args", event.GetProperty("args"))
                               : InterceptResult{};
  };
  for (const char* source :
       {"Object.defineProperty([], '0', {value: function() {}})",
        "Object.defineProperty([], '0', {value: Symbol('opaque')})",
        "(() => { const a = []; Object.defineProperty(a, '0', {value: a}); "
        "return a; })()",
        "(() => { const p = Object.create(Array.prototype); "
        "Object.defineProperty(p, '0', {value: function() {}}); "
        "const a = []; Object.setPrototypeOf(a, p); a.length = 1; "
        "return a; })()",
        "(() => { const a = {}; a.items = [{parent: a}]; return a; })()",
        "Object.assign(new ArrayBuffer(4), {then: function() {}})",
        "new (class Point { constructor() { this.x = 1; } })()"}) {
    auto value = Evaluate(source);
    EXPECT_FALSE(SnapshotJSValue(rt, value)) << source;
    Value args[] = {Value(rt, value)};
    (void)Call(args, 1);
    auto event = provider->Events(Kind::kCall).back();
    ASSERT_EQ(event.GetProperty("opaqueIndices").GetLength(), 1) << source;
  }
}

TEST_F(HostScriptModuleInterceptorTest, NonEnumerableArrayDataIsCopied) {
  auto snapshot = SnapshotJSValue(
      rt, Evaluate("Object.defineProperty([], '0', {value: {answer: 42}})"));
  ASSERT_TRUE(snapshot);
  EXPECT_EQ(snapshot->GetProperty(0).GetProperty("answer").Number(), 42);
}

TEST_F(HostScriptModuleInterceptorTest, SnapshotDoesNotInvokeAccessors) {
  auto value = Evaluate(
      "globalThis.snapshotReads = 0; ({get value() { "
      "return ++snapshotReads === 1 ? 42 : function() {}; }})");
  auto snapshot = SnapshotJSValue(rt, value);
  EXPECT_FALSE(snapshot);
  EXPECT_EQ(rt.global().getProperty(rt, "snapshotReads")->getNumber(), 0);
}

TEST_F(HostScriptModuleInterceptorTest,
       AccessorDescriptorDoesNotReadInheritedValue) {
  auto value = Evaluate(
      "globalThis.reads = 0; globalThis.inheritedReads = 0; "
      "Object.defineProperty(Object.prototype, 'value', "
      "{configurable: true, get() { ++inheritedReads; return 42; }}); "
      "({get answer() { return ++reads; }})");
  auto snapshot = SnapshotJSValue(rt, value);
  Evaluate("delete Object.prototype.value");
  EXPECT_FALSE(snapshot);
  EXPECT_EQ(rt.global().getProperty(rt, "reads")->getNumber(), 0);
  EXPECT_EQ(rt.global().getProperty(rt, "inheritedReads")->getNumber(), 0);
  Value args[] = {Value(rt, value)};
  ASSERT_TRUE(Call(args, 1).has_value());
  EXPECT_EQ(native->arguments.GetProperty(0).GetProperty("answer").Number(), 1);
}

TEST_F(HostScriptModuleInterceptorTest, ObservationDoesNotReadGettersTwice) {
  Value args[] = {
      Evaluate("globalThis.reads = 0; ({get value() { return ++reads; }})")};
  ASSERT_TRUE(Call(args, 1).has_value());
  EXPECT_EQ(rt.global().getProperty(rt, "reads")->getNumber(), 1);
  EXPECT_EQ(native->arguments.GetProperty(0).GetProperty("value").Number(), 1);
  auto event = provider->Events(Kind::kCall).back();
  ASSERT_EQ(event.GetProperty("opaqueIndices").GetLength(), 1);
  EXPECT_EQ(event.GetProperty("opaqueIndices").GetProperty(0).Number(), 0);
}

TEST_F(HostScriptModuleInterceptorTest, FailedHandlerDoesNotReadGettersTwice) {
  provider->dispatch = [](Kind, const lepus::Value&) {
    InterceptResult result;
    result.failed = true;
    return result;
  };
  Value args[] = {
      Evaluate("globalThis.reads = 0; ({get value() { return ++reads; }})")};
  ASSERT_TRUE(Call(args, 1).has_value());
  EXPECT_EQ(rt.global().getProperty(rt, "reads")->getNumber(), 1);
  EXPECT_EQ(native->arguments.GetProperty(0).GetProperty("value").Number(), 1);
}

TEST_F(HostScriptModuleInterceptorTest,
       GetterIsKeptWhenOtherArgumentIsPatched) {
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    if (kind != Kind::kCall) return InterceptResult{};
    auto args = lepus::CArray::Create();
    args->push_back(event.GetProperty("args").GetProperty(0));
    args->push_back(lepus::Value(9));
    return Patch("args", lepus::Value(args));
  };
  Value args[] = {
      Evaluate("globalThis.reads = 0; ({get value() { return ++reads; }})"),
      Value(1)};
  ASSERT_TRUE(Call(args, 2).has_value());
  EXPECT_EQ(rt.global().getProperty(rt, "reads")->getNumber(), 1);
  EXPECT_EQ(native->arguments.GetProperty(0).GetProperty("value").Number(), 1);
  EXPECT_EQ(native->arguments.GetProperty(1).Number(), 9);
}

TEST_F(HostScriptModuleInterceptorTest, NestedAndArrayAccessorsStayOpaque) {
  for (const char* source :
       {"globalThis.reads = 0; ({nested: {get value() { return ++reads; }}})",
        "globalThis.reads = 0; Object.defineProperty([], '0', "
        "{get() { return ++reads; }})",
        "globalThis.reads = 0; (() => { const p = "
        "Object.create(Array.prototype); "
        "Object.defineProperty(p, '0', {get() { return ++reads; }}); "
        "const a = []; Object.setPrototypeOf(a, p); a.length = 1; return a; "
        "})()"}) {
    auto value = Evaluate(source);
    EXPECT_FALSE(SnapshotJSValue(rt, value)) << source;
    EXPECT_EQ(rt.global().getProperty(rt, "reads")->getNumber(), 0) << source;
    Value args[] = {Value(rt, value)};
    ASSERT_TRUE(Call(args, 1).has_value());
    EXPECT_EQ(rt.global().getProperty(rt, "reads")->getNumber(), 1) << source;
    EXPECT_EQ(provider->Events(Kind::kCall)
                  .back()
                  .GetProperty("opaqueIndices")
                  .GetLength(),
              1)
        << source;
  }
}

TEST_F(HostScriptModuleInterceptorTest, MixedContainersKeepSparseAndShared) {
  auto snapshot = SnapshotJSValue(
      rt, Evaluate("(() => { const shared = {answer: 42}; return {items: "
                   "[shared, , null, {nested: [shared]}], bytes: new "
                   "ArrayBuffer(4)}; })()"));
  ASSERT_TRUE(snapshot);
  auto items = snapshot->GetProperty("items");
  ASSERT_EQ(items.GetLength(), 4);
  EXPECT_EQ(items.GetProperty(0).GetProperty("answer").Number(), 42);
  EXPECT_TRUE(items.GetProperty(1).IsUndefined());
  EXPECT_TRUE(items.GetProperty(2).IsNil());
  EXPECT_EQ(items.GetProperty(3)
                .GetProperty("nested")
                .GetProperty(0)
                .GetProperty("answer")
                .Number(),
            42);
  EXPECT_TRUE(snapshot->GetProperty("bytes").IsByteArray());
}

TEST_F(HostScriptModuleInterceptorTest, PlatformValuesAreCheckedForLoss) {
  using Type = CallbackValue::Type;
  for (auto type : {Type::kTransfer, Type::kLynxObject, Type::kTemplateData,
                    Type::kUnknown}) {
    for (int nesting = 0; nesting < 3; ++nesting) {
      std::unique_ptr<pub::Value> child = std::make_unique<CallbackValue>(type);
      if (nesting > 0) {
        child = std::make_unique<CallbackValue>(
            nesting == 1 ? Type::kArray : Type::kMap, std::move(child));
      }
      EXPECT_FALSE(CanSnapshot(CallbackValue(Type::kArray, std::move(child))));
    }
  }
  // An array that contains itself.
  EXPECT_FALSE(CanSnapshot(CallbackValue(Type::kArray)));
  EXPECT_TRUE(CanSnapshot(
      CallbackValue(Type::kArray, std::make_unique<pub::ValueImplLepus>(42))));
}

#if ENABLE_TESTBENCH_RECORDER
TEST_F(HostScriptModuleInterceptorTest, MockCallbacksKeepFixtureAssociation) {
  using namespace tasm::recorder;
  auto& recorder = TestBenchBaseRecorder::GetInstance();
  auto flush = [&] {
    fml::AutoResetWaitableEvent done;
    recorder.thread_.GetTaskRunner()->PostTask([&] { done.Signal(); });
    done.Wait();
  };
  flush();
  recorder.ResetForTesting();
  struct Reset {
    TestBenchBaseRecorder& recorder;
    ~Reset() { recorder.ResetForTesting(); }
  } reset{recorder};
  constexpr int64_t record_id = 62410;
  recorder.InitConfig("", 1, 390, 844, record_id);
  flush();
  recorder.StartRecord(ArtifactFormat::kBoth);
  module->SetRecordID(record_id);
  MockCall({MockCallback(1, Array(lepus::Value(20))),
            MockCallback(0, Array(lepus::Value(10)))});
  std::vector<double> values;
  Value callback = Callback(values);
  Value args[] = {Value(rt, callback), Value(rt, callback), Callback(values)};
  for (size_t invocation = 0; invocation < 2; ++invocation) {
    ASSERT_TRUE(Call(args, 3).has_value());
    EXPECT_EQ(values.size(), invocation * 2);
    ASSERT_EQ(delegate->pending.size(), 2u);
    delegate->Drain();
    EXPECT_TRUE(delegate->functions.empty());
  }
  flush();
  EXPECT_EQ(values, (std::vector<double>{20, 10, 20, 10}));
  ASSERT_EQ(recorder.fixture_table_.count(record_id), 1u);
  auto& groups = recorder.fixture_table_[record_id].call_groups;
  ASSERT_EQ(groups.size(), 1u);
  ASSERT_EQ(groups[0].calls.size(), 2u);
  for (auto& call : groups[0].calls) {
    ASSERT_EQ(call.callbacks.size(), 2u);
    for (size_t i = 0; i < call.callbacks.size(); ++i) {
      auto& recorded_callback = call.callbacks[i];
      EXPECT_EQ(recorded_callback.index, 1 - i);
      rapidjson::Document value;
      value.Parse(recorded_callback.value_json);
      ASSERT_TRUE(value.IsNumber());
      EXPECT_DOUBLE_EQ(value.GetDouble(), (2 - i) * 10);
    }
  }
  // Each registered callback keeps the legacy single-response object shape.
  auto& recorded = recorder.lynx_view_table_[record_id][kCallback];
  ASSERT_EQ(recorded.MemberCount(), 4u);
  for (const char* id : {"1", "2", "4", "5"}) {
    ASSERT_TRUE(recorded.HasMember(id));
    EXPECT_TRUE(recorded[id].IsObject());
  }
}
#endif

}  // namespace
}  // namespace lynx::shell
