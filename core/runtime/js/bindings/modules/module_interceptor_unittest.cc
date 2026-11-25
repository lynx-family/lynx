// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/module_interceptor.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/value/array.h"
#include "core/inspector/observer/native_module_record_observer.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module_callback.h"
#include "core/runtime/js/bindings/modules/module_interceptor_registry.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace js {
namespace {

class NativeModule : public LynxNativeModule {
 public:
  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string&, std::unique_ptr<pub::Value> args, size_t,
      const CallbackMap& callbacks) override {
    ++calls;
    arguments = pub::ValueUtils::ConvertValueToLepusValue(*args);
    saved_callbacks = callbacks;
    return std::unique_ptr<pub::Value>(new pub::ValueImplLepus(result));
  }
  lepus::Value result = lepus::Value(42);
  int calls = 0;
  lepus::Value arguments;
  CallbackMap saved_callbacks;
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
    if (id_to_delete != ModuleCallback::kInvalidCallbackId) {
      functions.erase(id_to_delete);
    }
    if (!callback) return;
    auto entry = functions.find(callback->callback_id());
    ASSERT_NE(entry, functions.end());
    auto holder = std::move(entry->second);
    functions.erase(entry);
    callback->Invoke(&rt, holder.get());
  }
  Runtime& rt;
  int64_t next_id = 0;
  std::unordered_map<int64_t, std::unique_ptr<ModuleCallbackFunctionHolder>>
      functions;
};

class RecordContext : public NativeModuleInvocationContext {
 public:
  explicit RecordContext(std::optional<lepus::Value>& result)
      : result(result) {}
  std::shared_ptr<NativeModuleInvocationContext> WithCallbackArgumentIndex(
      int32_t) const override {
    return std::make_shared<RecordContext>(result);
  }
  void OnInvoke(lepus::Value, const CallbackMap&, bool,
                std::optional<lepus::Value> value, int32_t,
                const std::string&) const override {
    result = std::move(value);
  }
  void OnCallback(lepus::Value) const override {}
  std::optional<lepus::Value>& result;
};

class Observer : public NativeModuleRecordObserver {
 public:
  void OnRecord(const lepus::Value&) override {}
  void OnGlobalEvent(const std::string&, const lepus::Value&) override {}
  std::shared_ptr<NativeModuleInvocationContext> CreateInvocation(
      const std::string&, const std::string&) override {
    return std::make_shared<RecordContext>(result);
  }
  std::optional<lepus::Value> result;
};

struct Tag : ModuleCallContext {
  explicit Tag(std::string name) : name(std::move(name)) {}
  std::string name;
};

// Records every hook as "<name>.<hook>" and runs optional behaviors.
class Recorder : public ModuleInterceptor {
 public:
  Recorder(std::string name, std::vector<std::string>& events)
      : name(std::move(name)), events(events) {}
  std::shared_ptr<ModuleCallContext> BeforeConvertArgs(
      const std::shared_ptr<LynxModule>&, const LynxModule::MethodMetadata&,
      Runtime*, const Value* args, size_t count,
      std::vector<Value>& rewritten) override {
    Log("start");
    if (on_start) on_start(args, count, rewritten);
    return provides_context ? std::make_shared<Tag>(name) : nullptr;
  }
  void BeforeInvokeMethod(const LynxModule::MethodMetadata&,
                          const std::unique_ptr<pub::Value>&,
                          const NativeModuleInfoCollectorPtr&) override {
    Log("before");
  }
  ModuleInterceptorResult InterceptModuleMethod(
      const std::shared_ptr<LynxModule>&, const LynxModule::MethodMetadata&,
      Runtime*, const std::shared_ptr<ModuleDelegate>&, const Value*, size_t,
      const std::unique_ptr<pub::Value>&, const CallbackMap& callbacks,
      NativeModuleInfoCollectorPtr, ModuleCallContext* context) const override {
    Log("intercept");
    auto result =
        on_intercept ? on_intercept(callbacks, context) : std::nullopt;
    return result ? ModuleInterceptorResult{true, std::move(*result)}
                  : ModuleInterceptorResult{false, Value::null()};
  }
  bool AfterInvokeMethod(Runtime*, ModuleCallContext* context,
                         Value& result) override {
    Log("result");
    if (provides_context) EXPECT_EQ(static_cast<Tag*>(context)->name, name);
    if (replace_with_undefined) {
      result = Value::undefined();
      return true;
    }
    if (!replace_result) return false;
    result = Value(*replace_result);
    return true;
  }
  void OnInvokeMethodEnd(ModuleCallContext* context,
                         const std::shared_ptr<ModuleDelegate>&,
                         const CallbackMap& callbacks,
                         bool converted) override {
    Log("end");
    if (provides_context) EXPECT_EQ(static_cast<Tag*>(context)->name, name);
    if (on_end) on_end(context, callbacks, converted);
  }
  void WillInvokeCallback(ModuleCallback& callback,
                          std::unique_ptr<pub::Value>& args) override {
    Log("will_callback");
    if (provides_context) {
      auto* tag = static_cast<Tag*>(callback.call_context().get());
      EXPECT_TRUE(tag && tag->name == name);
    }
    argument_index = callback.argument_index();
    if (replace_callback) {
      auto array = lepus::CArray::Create();
      array->emplace_back(*replace_callback);
      args = std::make_unique<pub::ValueImplLepus>(lepus::Value(array));
    }
  }
  void OnCallbackInvoked(const NativeModuleInfoCollectorPtr&,
                         ModuleCallback*) override {
    Log("callback");
  }
  void Log(const char* hook) const { events.push_back(name + "." + hook); }

  std::string name;
  bool provides_context = false;
  std::vector<std::string>& events;
  std::function<void(const Value*, size_t, std::vector<Value>&)> on_start;
  std::function<std::optional<Value>(const CallbackMap&, ModuleCallContext*)>
      on_intercept;
  std::function<void(ModuleCallContext*, const CallbackMap&, bool)> on_end;
  bool replace_with_undefined = false;
  std::optional<double> replace_result;
  std::optional<double> replace_callback;
  int argument_index = -1;
};

class ModuleInterceptorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto vm = rt.createVM(nullptr);
    auto context = rt.createContext(vm);
    rt.InitRuntime(context);
    module->SetModuleInterceptor(group);
    module->SetNativeModuleRecordObserver(observer);
    native->SetDelegate(module);
  }
  Recorder* Add(const std::string& name) {
    auto recorder = std::make_unique<Recorder>(name, events);
    recorder->provides_context = !has_context_provider;
    has_context_provider = true;
    auto* pointer = recorder.get();
    group->AddInterceptor(std::move(recorder), true);
    return pointer;
  }
  base::expected<Value, JSINativeException> Call(const Value* args,
                                                 size_t count) {
    return module->invokeMethod(LynxModule::MethodMetadata(count, "echo"), &rt,
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
  void Deliver(const std::shared_ptr<LynxModuleCallback>& callback,
               double value) {
    auto array = lepus::CArray::Create();
    array->emplace_back(value);
    callback->SetArgs(
        std::make_unique<pub::ValueImplLepus>(lepus::Value(array)));
    module->InvokeCallback(callback);
  }

  QuickjsRuntime rt;
  bool has_context_provider = false;
  std::vector<std::string> events;
  std::shared_ptr<Delegate> delegate = std::make_shared<Delegate>(rt);
  std::shared_ptr<NativeModule> native = std::make_shared<NativeModule>();
  std::shared_ptr<LynxJSIModule> module =
      std::make_shared<LynxJSIModule>("Example", delegate, native);
  std::shared_ptr<GroupInterceptor> group =
      std::make_shared<GroupInterceptor>();
  std::shared_ptr<Observer> observer = std::make_shared<Observer>();
};

TEST_F(ModuleInterceptorTest, LifecycleOrder) {
  Add("a");
  Add("b");
  Value args[] = {Value(1)};
  auto result = Call(args, 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 42);
  EXPECT_EQ(native->calls, 1);
  EXPECT_EQ(events,
            (std::vector<std::string>{
                "a.start", "b.start", "a.before", "b.before", "a.intercept",
                "b.intercept", "a.result", "b.result", "a.end", "b.end"}));
}

TEST_F(ModuleInterceptorTest, StartReplacesArgumentsBeforeConversion) {
  Add("a")->on_start = [this](const Value* args, size_t,
                              std::vector<Value>& rewritten) {
    rewritten.emplace_back(Value(5));
    rewritten.emplace_back(Value(rt, args[1]));
  };
  Value args[] = {Value(1), Value(true)};
  ASSERT_TRUE(Call(args, 2).has_value());
  EXPECT_EQ(native->arguments.GetProperty(0).Number(), 5);
  EXPECT_TRUE(native->arguments.GetProperty(1).Bool());
}

TEST_F(ModuleInterceptorTest, EndRunsWhenConversionFails) {
  Add("a");
  auto cyclic = rt.global()
                    .getPropertyAsFunction(rt, "eval")
                    ->call(rt, {Value(rt, String::createFromAscii(
                                              rt, "var o = {}; o.o = o; o"))});
  ASSERT_TRUE(cyclic && cyclic->isObject());
  Value args[] = {Value(rt, *cyclic)};
  EXPECT_FALSE(Call(args, 1).has_value());
  EXPECT_EQ(native->calls, 0);
  EXPECT_EQ(events, (std::vector<std::string>{"a.start", "a.end"}));
}

TEST_F(ModuleInterceptorTest, InterceptStopsPropagation) {
  Add("a");
  Add("b")->on_intercept = [](const CallbackMap& callbacks,
                              ModuleCallContext*) {
    EXPECT_EQ(callbacks.size(), 1u);
    return std::optional<Value>(Value(7));
  };
  Add("c");
  std::vector<double> values;
  Value args[] = {Callback(values)};
  auto result = Call(args, 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 7);
  EXPECT_EQ(native->calls, 0);
  // Resolved values are recorded like native results.
  ASSERT_TRUE(observer->result.has_value());
  EXPECT_EQ(observer->result->Number(), 7);
  EXPECT_EQ(events, (std::vector<std::string>{
                        "a.start", "b.start", "c.start", "a.before", "b.before",
                        "c.before", "a.intercept", "b.intercept", "a.result",
                        "b.result", "a.end", "b.end", "c.end"}));
}

TEST_F(ModuleInterceptorTest, ResultReplacementIsRecorded) {
  Add("a")->replace_result = 9;
  auto result = Call(nullptr, 0);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 9);
  ASSERT_TRUE(observer->result.has_value());
  EXPECT_EQ(observer->result->Number(), 9);
}

TEST_F(ModuleInterceptorTest, UndefinedReplacementClearsRecordedResult) {
  Add("a")->replace_with_undefined = true;
  auto result = Call(nullptr, 0);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->isUndefined());
  EXPECT_FALSE(observer->result.has_value());
}

TEST_F(ModuleInterceptorTest, EndSeesStateAndCallbacksAfterNativeConsumesArgs) {
  auto* recorder = Add("a");
  bool ended = false;
  recorder->on_end = [&](ModuleCallContext* context,
                         const CallbackMap& callbacks, bool converted) {
    ended = true;
    EXPECT_TRUE(converted);
    ASSERT_EQ(callbacks.size(), 1u);
    auto callback = std::static_pointer_cast<ModuleCallback>(callbacks.at(1));
    EXPECT_EQ(callback->call_context().get(), context);
    EXPECT_EQ(static_cast<Tag*>(context)->name, "a");
  };
  std::vector<double> values;
  Value args[] = {Value(5), Callback(values)};
  ASSERT_TRUE(Call(args, 2).has_value());
  EXPECT_TRUE(ended);
  EXPECT_EQ(native->arguments.GetProperty(0).Number(), 5);
  // The stack cleanup has run, but the callback still owns the same state.
  Deliver(native->saved_callbacks.at(1), 9);
  EXPECT_EQ(values, std::vector<double>{9});
}

TEST_F(ModuleInterceptorTest, ArgumentReplacementsComposeInOrder) {
  Add("a")->on_start = [](const Value*, size_t, std::vector<Value>& rewritten) {
    rewritten.emplace_back(5);
  };
  Add("b")->on_start = [](const Value* args, size_t,
                          std::vector<Value>& rewritten) {
    rewritten.emplace_back(args[0].getNumber() + 1);
  };
  Value args[] = {Value(1)};
  ASSERT_TRUE(Call(args, 1).has_value());
  EXPECT_EQ(native->arguments.GetProperty(0).Number(), 6);
  EXPECT_EQ(args[0].getNumber(), 1);
}

TEST_F(ModuleInterceptorTest, InvalidReplacementKeepsArgumentCount) {
  Add("a")->on_start = [](const Value*, size_t, std::vector<Value>& rewritten) {
    rewritten.emplace_back(5);
    rewritten.emplace_back(6);
  };
  Value args[] = {Value(1)};
  ASSERT_TRUE(Call(args, 1).has_value());
  EXPECT_EQ(native->arguments.GetLength(), 1);
  EXPECT_EQ(native->arguments.GetProperty(0).Number(), 1);
}

TEST_F(ModuleInterceptorTest, CallbackHooksSeeTheirInvocation) {
  auto* a = Add("a");
  a->replace_callback = 3;
  std::vector<double> values;
  Value args[] = {Value(0), Callback(values)};
  ASSERT_TRUE(Call(args, 2).has_value());
  events.clear();
  Deliver(native->saved_callbacks.at(1), 1);
  EXPECT_EQ(values, std::vector<double>{3});
  EXPECT_EQ(a->argument_index, 1);
  EXPECT_EQ(events,
            (std::vector<std::string>{"a.will_callback", "a.callback"}));
}

TEST_F(ModuleInterceptorTest,
       ContextIsSharedWithinOneCallAndReleasedWithCallbacks) {
  Add("a");
  std::vector<double> values;
  Value args[] = {Callback(values), Callback(values)};
  ASSERT_TRUE(Call(args, 2).has_value());
  auto first = native->saved_callbacks;
  auto context_of = [](const std::shared_ptr<LynxModuleCallback>& callback) {
    return std::static_pointer_cast<ModuleCallback>(callback)->call_context();
  };
  std::weak_ptr<ModuleCallContext> first_context = context_of(first.at(0));
  EXPECT_EQ(context_of(first.at(0)), context_of(first.at(1)));
  ASSERT_TRUE(Call(args, 2).has_value());
  EXPECT_NE(first_context.lock(), context_of(native->saved_callbacks.at(0)));

  // Deliver the newer invocation first, after both synchronous calls ended.
  Deliver(native->saved_callbacks.at(0), 20);
  Deliver(first.at(1), 11);
  Deliver(first.at(0), 10);
  EXPECT_EQ(values, (std::vector<double>{20, 11, 10}));
  first.erase(0);
  EXPECT_FALSE(first_context.expired());
  first.clear();
  EXPECT_TRUE(first_context.expired());
}

TEST_F(ModuleInterceptorTest, EmptyGroupDoesNotCreateContext) {
  EXPECT_FALSE(group->HasInvocationHooks());
  std::vector<Value> rewritten;
  EXPECT_EQ(
      group->BeforeConvertArgs(module, LynxModule::MethodMetadata(0, "echo"),
                               &rt, nullptr, 0, rewritten),
      nullptr);
  ASSERT_TRUE(Call(nullptr, 0).has_value());
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  auto callback =
      std::static_pointer_cast<ModuleCallback>(native->saved_callbacks.at(0));
  EXPECT_EQ(callback->call_context(), nullptr);
  Deliver(callback, 7);
  EXPECT_EQ(values, std::vector<double>{7});
}

TEST_F(ModuleInterceptorTest, LegacyChainSkipsNewHooksAndContext) {
  auto recorder = std::make_unique<Recorder>("legacy", events);
  recorder->replace_result = 99;
  recorder->replace_callback = 99;
  recorder->on_intercept = [](const CallbackMap&, ModuleCallContext* context) {
    EXPECT_EQ(context, nullptr);
    return std::optional<Value>();
  };
  group->AddInterceptor(std::move(recorder));
  EXPECT_FALSE(group->HasInvocationHooks());
  auto result = Call(nullptr, 0);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 42);
  EXPECT_EQ(events,
            (std::vector<std::string>{"legacy.before", "legacy.intercept"}));
  events.clear();
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  auto callback =
      std::static_pointer_cast<ModuleCallback>(native->saved_callbacks.at(0));
  EXPECT_EQ(callback->call_context(), nullptr);
  Deliver(callback, 7);
  EXPECT_EQ(values, std::vector<double>{7});
  EXPECT_EQ(events,
            (std::vector<std::string>{"legacy.before", "legacy.intercept",
                                      "legacy.callback"}));
}

TEST_F(ModuleInterceptorTest, LegacyHandledResultIsStillRecorded) {
  auto recorder = std::make_unique<Recorder>("legacy", events);
  recorder->on_intercept = [](const CallbackMap&, ModuleCallContext* context) {
    EXPECT_EQ(context, nullptr);
    return std::optional<Value>(Value(7));
  };
  group->AddInterceptor(std::move(recorder));
  auto result = Call(nullptr, 0);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getNumber(), 7);
  EXPECT_EQ(native->calls, 0);
  ASSERT_TRUE(observer->result.has_value());
  EXPECT_EQ(observer->result->Number(), 7);
}

TEST_F(ModuleInterceptorTest, LegacyConversionFailureSkipsNewCleanup) {
  group->AddInterceptor(std::make_unique<Recorder>("legacy", events));
  Object cyclic(rt);
  cyclic.setProperty(rt, "self", Value(rt, cyclic));
  Value args[] = {Value(rt, cyclic)};
  EXPECT_FALSE(Call(args, 1).has_value());
  EXPECT_TRUE(events.empty());
  EXPECT_EQ(native->calls, 0);
}

TEST_F(ModuleInterceptorTest, LeadingRegistrationEnablesInvocationHooks) {
  group->AddInterceptor(std::make_unique<Recorder>("legacy", events));
  EXPECT_FALSE(group->HasInvocationHooks());
  auto leading = std::make_unique<Recorder>("leading", events);
  leading->provides_context = true;
  group->AddLeadingInterceptor(std::move(leading));
  EXPECT_TRUE(group->HasInvocationHooks());
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  auto callback =
      std::static_pointer_cast<ModuleCallback>(native->saved_callbacks.at(0));
  ASSERT_NE(callback->call_context(), nullptr);
  EXPECT_EQ(static_cast<Tag*>(callback->call_context().get())->name, "leading");
  EXPECT_EQ(events.front(), "leading.start");
  Deliver(callback, 7);
  EXPECT_EQ(values, std::vector<double>{7});
}

TEST_F(ModuleInterceptorTest, StatelessChainStillBoundsInlineCallbacks) {
  auto* handler = Add("a");
  handler->provides_context = false;
  handler->on_intercept = [&](const CallbackMap& callbacks,
                              ModuleCallContext* context) {
    EXPECT_NE(context, nullptr);
    EXPECT_EQ(context->resolver(), handler);
    Deliver(callbacks.at(0), 7);
    return std::optional<Value>(Value(7));
  };
  Add("b")->replace_callback = 99;
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  EXPECT_EQ(values, std::vector<double>{7});
}

TEST(ModuleInterceptorDeathTest, RejectsMultipleContextProviders) {
  GroupInterceptor group;
  std::vector<std::string> events;
  for (const char* name : {"a", "b"}) {
    auto interceptor = std::make_unique<Recorder>(name, events);
    interceptor->provides_context = true;
    group.AddInterceptor(std::move(interceptor), true);
  }
  std::vector<Value> rewritten;
  EXPECT_DEATH_IF_SUPPORTED(
      group.BeforeConvertArgs(nullptr, LynxModule::MethodMetadata(0, "echo"),
                              nullptr, nullptr, 0, rewritten),
      "");
}

TEST_F(ModuleInterceptorTest, CallbackHooksStopAtResolver) {
  Add("a");
  CallbackMap callbacks;
  Add("b")->on_intercept = [&](const CallbackMap& registered,
                               ModuleCallContext*) {
    callbacks = registered;
    return std::optional<Value>(Value::undefined());
  };
  Add("c");
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  events.clear();
  Deliver(callbacks.at(0), 1);
  EXPECT_EQ(values, std::vector<double>{1});
  EXPECT_EQ(events,
            (std::vector<std::string>{"a.will_callback", "b.will_callback",
                                      "a.callback", "b.callback"}));
}

TEST_F(ModuleInterceptorTest, InlineCallbackHooksStopAtHandlingInterceptor) {
  Add("a");
  auto* handler = Add("b");
  handler->on_intercept = [&](const CallbackMap& callbacks,
                              ModuleCallContext* context) {
    EXPECT_EQ(context->resolver(), handler);
    Deliver(callbacks.at(0), 7);
    return std::optional<Value>(Value(7));
  };
  Add("c")->replace_callback = 99;
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  EXPECT_EQ(values, std::vector<double>{7});
  EXPECT_EQ(std::count(events.begin(), events.end(), "c.will_callback"), 0);
  EXPECT_EQ(std::count(events.begin(), events.end(), "c.callback"), 0);
}

TEST_F(ModuleInterceptorTest, ReentrantInterceptionKeepsOuterStateAndBoundary) {
  auto* handler = Add("a");
  ModuleCallContext* outer_record = nullptr;
  bool nested = false;
  handler->on_intercept = [&](const CallbackMap& callbacks,
                              ModuleCallContext* context) {
    EXPECT_EQ(context->resolver(), handler);
    if (nested) {
      EXPECT_NE(context, outer_record);
      return std::optional<Value>(Value(2));
    }
    outer_record = context;
    nested = true;
    EXPECT_TRUE(Call(nullptr, 0).has_value());
    EXPECT_EQ(context->resolver(), handler);
    Deliver(callbacks.at(0), 7);
    return std::optional<Value>(Value(1));
  };
  Add("b")->replace_callback = 99;
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  EXPECT_EQ(values, std::vector<double>{7});
}

TEST_F(ModuleInterceptorTest,
       DeclinedInterceptorsLeaveNativeCallbacksUnbounded) {
  Add("a");
  auto* tail = Add("b");
  tail->replace_callback = 9;
  std::vector<double> values;
  Value args[] = {Callback(values)};
  ASSERT_TRUE(Call(args, 1).has_value());
  auto callback =
      std::static_pointer_cast<ModuleCallback>(native->saved_callbacks.at(0));
  EXPECT_EQ(callback->call_context()->resolver(), nullptr);
  Deliver(callback, 7);
  EXPECT_EQ(values, std::vector<double>{9});
}

bool registry_enabled = false;
std::vector<std::string>* registry_events = nullptr;
std::unique_ptr<ModuleInterceptor> CreateLeading() {
  if (!registry_enabled) return nullptr;
  return std::make_unique<Recorder>("leading", *registry_events);
}

TEST_F(ModuleInterceptorTest, RegistryInstallsLeadingInterceptors) {
  registry_enabled = true;
  registry_events = &events;
  ModuleInterceptorRegistry::RegisterLeading(&CreateLeading);
  ModuleInterceptorRegistry::RegisterLeading(&CreateLeading);
  EXPECT_NE(ModuleInterceptorRegistry::Install(nullptr), nullptr);
  Add("a");
  EXPECT_EQ(ModuleInterceptorRegistry::Install(group), group);
  registry_enabled = false;
  // Creators may decline, leaving an empty configuration without a group.
  EXPECT_EQ(ModuleInterceptorRegistry::Install(nullptr), nullptr);
  EXPECT_EQ(
      ModuleInterceptorRegistry::Install(std::make_shared<GroupInterceptor>()),
      nullptr);
  EXPECT_TRUE(group->HasInvocationHooks());

  ASSERT_TRUE(Call(nullptr, 0).has_value());
  EXPECT_EQ(events.front(), "leading.start");
  EXPECT_EQ(std::count(events.begin(), events.end(), "leading.start"), 1);
  EXPECT_EQ(events[1], "a.start");
}

}  // namespace
}  // namespace js
}  // namespace runtime
}  // namespace lynx
