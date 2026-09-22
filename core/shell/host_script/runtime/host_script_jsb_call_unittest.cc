// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/host_script/runtime/host_script_jsb_call.h"

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::runtime::js {
namespace {
using Kind = shell::InterceptKind;
shell::InterceptResult Patch(const char* key, lepus::Value value) {
  shell::InterceptResult result;
  result.patch = lepus::Value(lepus::Dictionary::Create());
  result.patch.SetProperty(key, std::move(value));
  return result;
}
class Provider : public shell::Interceptor {
 public:
  bool HasHandlers(Kind kind) const override {
    ++queries;
    return enabled &&
           (kind == Kind::kCall || (kind == Kind::kResult && result_enabled) ||
            kind == Kind::kCallback);
  }
  shell::InterceptResult Dispatch(Kind kind,
                                  const lepus::Value& event) override {
    events.emplace_back(kind, event);
    return dispatch ? dispatch(kind, event) : shell::InterceptResult{};
  }
  void ReportError(const std::string&) override {}
  bool enabled = true;
  bool result_enabled = true;
  mutable int queries = 0;
  std::function<shell::InterceptResult(Kind, const lepus::Value&)> dispatch;
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

class HostScriptJSBCallTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto vm = rt.createVM(nullptr);
    auto context = rt.createContext(vm);
    rt.InitRuntime(context);
    ASSERT_TRUE(
        shell::Interceptor::Attach(provider, shell::Interceptor::Domain::kBTS));
    shell::Interceptor::RegisterView(kView);
  }
  void TearDown() override {
    shell::Interceptor::DestroyView(kView);
    shell::Interceptor::Detach(provider.get());
  }
  Value Callback(std::vector<double>& values) {
    return Value(Function::createFromHostFunction(
        rt, PropNameID::forAscii(rt, "callback"), 1,
        [&values](Runtime&, const Value&, const Value* args, size_t count) {
          if (count) values.push_back(args[0].getNumber());
          return Value::undefined();
        }));
  }
  static constexpr base::LynxEntityId kView = 42;
  QuickjsRuntime rt;
  std::shared_ptr<Provider> provider = std::make_shared<Provider>();
};
TEST_F(HostScriptJSBCallTest, OpaqueAndCallbackArgumentsAreNotReplaced) {
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
  auto invocation = HostScriptJSBCall::Create("Example", "echo", kView);
  std::vector<Value> replaced;
  invocation->OnCall(rt, args, 2, replaced);
  ASSERT_EQ(replaced.size(), 2u);
  EXPECT_TRUE(Value::strictEquals(rt, args[0], replaced[0]));
  EXPECT_TRUE(Value::strictEquals(rt, args[1], replaced[1]));
}
TEST_F(HostScriptJSBCallTest, MissingProviderReportsOneGapPerInvocation) {
  const auto before = shell::Interceptor::FailureCount();
  for (auto kind : {Kind::kCall, Kind::kResult, Kind::kCallback}) {
    shell::Interceptor::HandlerAdded(kind, shell::Interceptor::Domain::kBTS);
  }
  std::thread other([] {
    EXPECT_EQ(HostScriptJSBCall::Create("Example", "echo", kView), nullptr);
  });
  other.join();
  EXPECT_EQ(shell::Interceptor::FailureCount(), before + 1);
  for (auto kind : {Kind::kCall, Kind::kResult, Kind::kCallback}) {
    shell::Interceptor::HandlerRemoved(kind, shell::Interceptor::Domain::kBTS);
  }
}
TEST_F(HostScriptJSBCallTest, ArgumentPatchIsNotRetainedByCallContext) {
  auto payload = lepus::CArray::Create();
  payload->push_back(lepus::Value(9));
  provider->dispatch = [payload](Kind, const lepus::Value&) {
    return Patch("args", lepus::Value(payload));
  };
  auto call = HostScriptJSBCall::Create("Example", "echo", kView);
  ASSERT_TRUE(call);
  Value args[] = {Value(1)};
  std::vector<Value> replacement;
  auto mock = call->OnCall(rt, args, 1, replacement);
  provider->dispatch = nullptr;
  EXPECT_FALSE(mock.IsTable());
  EXPECT_TRUE(payload->HasOneRef());
  ASSERT_EQ(replacement.size(), 1u);
  EXPECT_EQ(replacement[0].getNumber(), 9);
}
TEST_F(HostScriptJSBCallTest, MockDecisionIsNotRetainedByCallContext) {
  auto payload = lepus::Dictionary::Create();
  provider->dispatch = [payload](Kind, const lepus::Value&) {
    shell::InterceptResult result;
    result.mock = lepus::Value(payload);
    return result;
  };
  auto call = HostScriptJSBCall::Create("Example", "echo", kView);
  std::vector<Value> replacement;
  auto mock = call->OnCall(rt, nullptr, 0, replacement);
  provider->dispatch = nullptr;
  ASSERT_TRUE(mock.IsTable());
  mock = lepus::Value();
  EXPECT_TRUE(payload->HasOneRef());
}

TEST_F(HostScriptJSBCallTest, ObjectLengthDoesNotControlSnapshotAllocation) {
  auto call = HostScriptJSBCall::Create("Example", "echo", kView);
  ASSERT_TRUE(call);
  for (double length : {100000000.0, -1.0}) {
    for (bool nested : {false, true}) {
      Object object(rt);
      object.setProperty(rt, "length", Value(length));
      object.setProperty(rt, "answer", Value(42));
      Value value(std::move(object));
      if (nested) {
        Object outer(rt);
        outer.setProperty(rt, "payload", std::move(value));
        value = Value(std::move(outer));
      }
      std::vector<Value> rewritten;
      call->OnCall(rt, &value, 1, rewritten);
      EXPECT_TRUE(rewritten.empty());
      auto snapshot =
          provider->events.back().second.GetProperty("args").GetProperty(0);
      if (nested) snapshot = snapshot.GetProperty("payload");
      EXPECT_EQ(snapshot.GetLength(), 2);
      EXPECT_EQ(snapshot.GetProperty("length").Number(), length);
      EXPECT_EQ(snapshot.GetProperty("answer").Number(), 42);

      EXPECT_FALSE(call->OnResult(rt, value));
      snapshot = provider->events.back().second.GetProperty("value");
      if (nested) snapshot = snapshot.GetProperty("payload");
      EXPECT_EQ(snapshot.GetLength(), 2);
      EXPECT_EQ(snapshot.GetProperty("length").Number(), length);
    }
  }
}

TEST_F(HostScriptJSBCallTest, ArrayIndicesAreCheckedEvenWhenNotEnumerable) {
  provider->dispatch = [](Kind, const lepus::Value& event) {
    return Patch("args", event.GetProperty("args"));
  };
  for (const char* source :
       {"Object.defineProperty([], '0', {value: function() {}})",
        "Object.defineProperty([], '0', {value: Symbol('opaque')})",
        "(() => { const a = []; Object.defineProperty(a, '0', {value: a}); "
        "return a; })()",
        "(() => { const p = Object.create(Array.prototype); "
        "Object.defineProperty(p, '0', {value: function() {}}); "
        "const a = []; Object.setPrototypeOf(a, p); a.length = 1; "
        "return a; })()"}) {
    auto value = rt.evaluateJavaScript(std::make_unique<StringBuffer>(source),
                                       "opaque-array.js");
    ASSERT_TRUE(value.has_value());
    auto call = HostScriptJSBCall::Create("Example", "echo", kView);
    std::vector<Value> rewritten;
    call->OnCall(rt, &*value, 1, rewritten);
    auto event = provider->events.back().second;
    ASSERT_EQ(event.GetProperty("opaqueIndices").GetLength(), 1);
    EXPECT_EQ(event.GetProperty("opaqueIndices").GetProperty(0).Number(), 0);
    ASSERT_EQ(rewritten.size(), 1u);
    EXPECT_TRUE(Value::strictEquals(rt, *value, rewritten[0]));
    auto event_count = provider->events.size();
    EXPECT_FALSE(call->OnResult(rt, *value));
    EXPECT_EQ(provider->events.size(), event_count);
  }
}

TEST_F(HostScriptJSBCallTest, NonEnumerableArrayDataCanBeCopied) {
  auto value = rt.evaluateJavaScript(
      std::make_unique<StringBuffer>(
          "Object.defineProperty([], '0', {value: {answer: 42}})"),
      "array-data.js");
  ASSERT_TRUE(value.has_value());
  auto call = HostScriptJSBCall::Create("Example", "echo", kView);
  std::vector<Value> rewritten;
  call->OnCall(rt, &*value, 1, rewritten);
  auto event = provider->events.back().second;
  EXPECT_EQ(event.GetProperty("opaqueIndices").GetLength(), 0);
  EXPECT_EQ(event.GetProperty("args")
                .GetProperty(0)
                .GetProperty(0)
                .GetProperty("answer")
                .Number(),
            42);
}

TEST_F(HostScriptJSBCallTest, SnapshotReadsEachGetterOnce) {
  for (bool result : {false, true}) {
    auto value = rt.evaluateJavaScript(
        std::make_unique<StringBuffer>(
            "globalThis.snapshotReads = 0; ({get value() { "
            "return ++snapshotReads === 1 ? 42 : function() {}; }})"),
        "snapshot-getter.js");
    ASSERT_TRUE(value.has_value());
    auto call = HostScriptJSBCall::Create("Example", "echo", kView);
    if (result) {
      EXPECT_FALSE(call->OnResult(rt, *value));
    } else {
      std::vector<Value> rewritten;
      call->OnCall(rt, &*value, 1, rewritten);
    }
    auto reads = rt.global().getProperty(rt, "snapshotReads");
    ASSERT_TRUE(reads);
    EXPECT_EQ(reads->getNumber(), 1);
    auto event = provider->events.back().second;
    auto snapshot = result ? event.GetProperty("value")
                           : event.GetProperty("args").GetProperty(0);
    EXPECT_EQ(snapshot.GetProperty("value").Number(), 42);
  }
}

TEST_F(HostScriptJSBCallTest, MixedContainersPreserveSparseAndSharedValues) {
  auto value = rt.evaluateJavaScript(
      std::make_unique<StringBuffer>(
          "(() => { const shared = {answer: 42}; return {items: "
          "[shared, , null, {nested: [shared]}], bytes: new ArrayBuffer(4)}; "
          "})()"),
      "mixed-containers.js");
  ASSERT_TRUE(value.has_value());
  auto call = HostScriptJSBCall::Create("Example", "echo", kView);
  std::vector<Value> rewritten;
  call->OnCall(rt, &*value, 1, rewritten);
  auto event = provider->events.back().second;
  EXPECT_EQ(event.GetProperty("opaqueIndices").GetLength(), 0);
  auto snapshot = event.GetProperty("args").GetProperty(0);
  auto items = snapshot.GetProperty("items");
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
  EXPECT_TRUE(snapshot.GetProperty("bytes").IsByteArray());
}

TEST_F(HostScriptJSBCallTest, NestedCyclesAndPromiseLikeBuffersStayOpaque) {
  provider->dispatch = [](Kind, const lepus::Value& event) {
    return Patch("args", event.GetProperty("args"));
  };
  for (const char* source :
       {"(() => { const a = {}; a.items = [{parent: a}]; return a; })()",
        "Object.assign(new ArrayBuffer(4), {then: function() {}})"}) {
    auto value = rt.evaluateJavaScript(std::make_unique<StringBuffer>(source),
                                       "opaque-containers.js");
    ASSERT_TRUE(value.has_value());
    auto call = HostScriptJSBCall::Create("Example", "echo", kView);
    std::vector<Value> rewritten;
    call->OnCall(rt, &*value, 1, rewritten);
    auto event = provider->events.back().second;
    ASSERT_EQ(event.GetProperty("opaqueIndices").GetLength(), 1);
    ASSERT_EQ(rewritten.size(), 1u);
    EXPECT_TRUE(Value::strictEquals(rt, *value, rewritten[0]));
    auto event_count = provider->events.size();
    EXPECT_FALSE(call->OnResult(rt, *value));
    EXPECT_EQ(provider->events.size(), event_count);
  }
}

TEST_F(HostScriptJSBCallTest, OpaqueCallbackValuesKeepOriginalArguments) {
  provider->dispatch = [](Kind, const lepus::Value& event) {
    return Patch("args", event.GetProperty("args"));
  };
  auto call = HostScriptJSBCall::Create("Example", "echo", kView);
  using Type = CallbackValue::Type;
  for (auto type : {Type::kTransfer, Type::kLynxObject, Type::kTemplateData,
                    Type::kUnknown}) {
    for (int nesting = 0; nesting < 3; ++nesting) {
      std::unique_ptr<pub::Value> child = std::make_unique<CallbackValue>(type);
      if (nesting > 0) {
        child = std::make_unique<CallbackValue>(
            nesting == 1 ? Type::kArray : Type::kMap, std::move(child));
      }
      std::unique_ptr<pub::Value> args =
          std::make_unique<CallbackValue>(Type::kArray, std::move(child));
      auto* original = args.get();
      call->OnCallback(0, args);
      EXPECT_EQ(args.get(), original);
      EXPECT_TRUE(provider->events.empty());
    }
  }
}

TEST_F(HostScriptJSBCallTest, CyclicCallbackIsNotConverted) {
  std::unique_ptr<pub::Value> args =
      std::make_unique<CallbackValue>(CallbackValue::Type::kArray);
  auto* original = args.get();
  auto call = HostScriptJSBCall::Create("Example", "echo", kView);
  call->OnCallback(0, args);
  EXPECT_EQ(args.get(), original);
  EXPECT_TRUE(provider->events.empty());
}

TEST_F(HostScriptJSBCallTest, SupportedPlatformCallbackCanStillBePatched) {
  provider->dispatch = [](Kind kind, const lepus::Value& event) {
    EXPECT_EQ(kind, Kind::kCallback);
    auto args = event.GetProperty("args");
    EXPECT_EQ(args.GetProperty(0).GetProperty("payload").Number(), 42);
    auto values = lepus::CArray::Create();
    values->push_back(lepus::Value(73));
    return Patch("args", lepus::Value(values));
  };
  using Type = CallbackValue::Type;
  std::unique_ptr<pub::Value> args = std::make_unique<CallbackValue>(
      Type::kArray, std::make_unique<CallbackValue>(
                        Type::kMap, std::make_unique<pub::ValueImplLepus>(42)));
  auto call = HostScriptJSBCall::Create("Example", "echo", kView);
  call->OnCallback(0, args);
  ASSERT_EQ(provider->events.size(), 1u);
  EXPECT_EQ(args->GetValueAtIndex(0)->Number(), 73);
}

}  // namespace
}  // namespace lynx::runtime::js
