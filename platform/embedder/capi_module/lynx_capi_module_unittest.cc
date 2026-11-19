// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/capi_module/lynx_capi_module.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "base/include/lynx_actor.h"
#include "core/runtime/lepus/bindings/modules/lepus_module_callback.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/lynx_engine.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/embedder/lynx_view_builder_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace embedder {
namespace {

class TestModuleCallback final : public runtime::LynxModuleCallback {
 public:
  TestModuleCallback() : LynxModuleCallback(1) {}
  void SetArgs(std::unique_ptr<pub::Value> args) override {
    args_ = std::move(args);
  }
  const pub::Value* args() const { return args_.get(); }

 private:
  std::unique_ptr<pub::Value> args_;
};

class TestModuleDelegate final : public runtime::LynxNativeModule::Delegate {
 public:
  void InvokeCallback(
      const std::shared_ptr<runtime::LynxModuleCallback>& callback,
      base::MoveOnlyClosure<bool> invoke_pre_func) override {
    base::closure task = [this, callback, func = std::move(invoke_pre_func)] {
      if (!func || func()) {
        ++invoke_count;
        last_callback = callback;
      }
    };
    if (defer_callbacks) {
      tasks.push_back(std::move(task));
    } else {
      task();
    }
  }

  void FlushCallbacks() {
    auto pending = std::move(tasks);
    for (auto& task : pending) {
      task();
    }
  }

  void RunOnJSThread(base::closure func) override { func(); }
  void RunOnPlatformThread(base::closure func) override { func(); }
  const std::shared_ptr<pub::PubValueFactory>& GetValueFactory() override {
    return value_factory;
  }
  void OnErrorOccurred(const std::string&, const std::string&,
                       base::LynxError) override {}

  bool defer_callbacks = false;
  int invoke_count = 0;
  std::shared_ptr<runtime::LynxModuleCallback> last_callback;
  std::vector<base::closure> tasks;
  std::shared_ptr<pub::PubValueFactory> value_factory =
      std::make_shared<pub::PubValueFactoryDefault>();
};

struct TestInvocationState {
  using Handler = std::function<int32_t(lynx_value*)>;
  ~TestInvocationState() {
    for (const auto& callback : callbacks) {
      lynx_native_module_callback_release(callback.callback);
    }
  }

  int invoke_count = 0;
  std::string method;
  std::vector<lepus::Value> arguments;
  std::vector<lynx_capi_module_callback_t> callbacks;
  Handler handler;
};

int32_t InvokeTestModule(void* opaque, const char* method_name,
                         const lynx_value* arguments, size_t argument_count,
                         const lynx_capi_module_callback_t* callbacks,
                         size_t callback_count, lynx_value* result) {
  auto& state = *static_cast<TestInvocationState*>(opaque);
  ++state.invoke_count;
  state.method = method_name;
  state.arguments.clear();
  for (size_t index = 0; index < argument_count; ++index) {
    // Capture the representation before any C++ getter can interpret it.
    EXPECT_NE(arguments[index].type, lynx_value_extended);
    state.arguments.emplace_back(nullptr, arguments[index]);
  }
  for (size_t index = 0; index < callback_count; ++index) {
    lynx_native_module_callback_retain(callbacks[index].callback);
    state.callbacks.push_back(callbacks[index]);
  }
  return state.handler ? state.handler(result) : LYNX_CAPI_MODULE_SUCCESS;
}

std::unique_ptr<pub::Value> CreateArguments(
    std::vector<lepus::Value> values = {}) {
  auto array = lepus::CArray::Create();
  for (auto& value : values) {
    array->push_back(std::move(value));
  }
  return std::make_unique<pub::ValueImplLepus>(lepus::Value(std::move(array)));
}

class LynxCapiModuleTest : public testing::Test {
 protected:
  void Register(size_t count = 0) {
    lynx_view_builder_t builder;
    const lynx_capi_module_method_t methods[] = {
        {.name = "call", .argument_count = count}};
    ASSERT_EQ(lynx_view_builder_register_capi_module(
                  &builder, "TestModule", methods, 1, InvokeTestModule, &state_,
                  nullptr),
              LYNX_CAPI_MODULE_SUCCESS);
    registry_ = std::make_shared<LynxCapiModuleRegistry>(
        std::move(builder.capi_modules));
    module_ = LynxCapiModuleFactory(registry_).CreateModule("TestModule");
    ASSERT_NE(module_, nullptr);
    module_->SetDelegate(delegate_);
  }

  void CreateCallback() {
    Register(1);
    auto response = module_->InvokeMethod(
        "call", CreateArguments({lepus::Value(int64_t{1})}), 1,
        {{0, callback_}});
    ASSERT_TRUE(response.has_value());
    ASSERT_EQ(state_.callbacks.size(), 1u);
  }

  lynx_native_module_callback_t* handle() {
    return state_.callbacks[0].callback;
  }

  TestInvocationState state_;
  std::shared_ptr<LynxCapiModuleRegistry> registry_;
  std::shared_ptr<runtime::LynxNativeModule> module_;
  std::shared_ptr<TestModuleDelegate> delegate_ =
      std::make_shared<TestModuleDelegate>();
  std::shared_ptr<TestModuleCallback> callback_ =
      std::make_shared<TestModuleCallback>();
};

TEST_F(LynxCapiModuleTest, FindsMethodsAndReturnsUndefinedByDefault) {
  Register();
  delegate_.reset();
  ASSERT_EQ(module_->GetMethodList().size(), 1u);
  EXPECT_EQ(module_->GetMethodList().at("call").args_count, 0u);
  EXPECT_EQ(LynxCapiModuleFactory(registry_).CreateModule("missing"), nullptr);
  auto response = module_->InvokeMethod("call", CreateArguments(), 0, {});
  ASSERT_TRUE(response.has_value());
  EXPECT_TRUE(response.value()->IsUndefined());
  EXPECT_EQ(state_.method, "call");
}

TEST_F(LynxCapiModuleTest, RejectsInvalidInvocationBeforeCallingConsumer) {
  Register(1);
  EXPECT_FALSE(
      module_->InvokeMethod("missing", CreateArguments(), 0, {}).has_value());
  EXPECT_FALSE(module_->InvokeMethod("call", nullptr, 1, {}).has_value());
  EXPECT_FALSE(
      module_->InvokeMethod("call", CreateArguments(), 1, {}).has_value());
  EXPECT_FALSE(
      module_->InvokeMethod("call", CreateArguments(), 0, {}).has_value());
  EXPECT_FALSE(module_
                   ->InvokeMethod("call", CreateArguments({lepus::Value(1)}), 1,
                                  {{-1, callback_}})
                   .has_value());
  EXPECT_FALSE(module_
                   ->InvokeMethod("call", CreateArguments({lepus::Value(1)}), 1,
                                  {{1, callback_}})
                   .has_value());
  EXPECT_FALSE(module_
                   ->InvokeMethod("call", CreateArguments({lepus::Value(1)}), 1,
                                  {{0, nullptr}})
                   .has_value());
  EXPECT_EQ(state_.invoke_count, 0);
}

TEST_F(LynxCapiModuleTest, PreservesIntegerWidthsAndBinaryValues) {
  Register(4);
  auto bytes = std::make_unique<uint8_t[]>(2);
  bytes[0] = 0;
  bytes[1] = 255;
  const int64_t number = (int64_t{1} << 60) + 1;
  auto response = module_->InvokeMethod(
      "call",
      CreateArguments(
          {lepus::Value(number), lepus::Value(uint32_t{42}),
           lepus::Value(std::numeric_limits<uint64_t>::max()),
           lepus::Value(lepus::ByteArray::Create(std::move(bytes), 2))}),
      4, {});
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(state_.arguments[0].value().type, lynx_value_int64);
  EXPECT_EQ(state_.arguments[0].Int64(), number);
  EXPECT_EQ(state_.arguments[1].value().type, lynx_value_uint32);
  EXPECT_EQ(state_.arguments[2].UInt64(), std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(state_.arguments[3].ByteArray()->GetLength(), 2u);
  EXPECT_EQ(state_.arguments[3].ByteArray()->GetPtr()[1], 255);
}

TEST_F(LynxCapiModuleTest, OwnsReturnedReferenceOnSuccessAndError) {
  Register();
  lepus::Value value("result");
  auto* string = reinterpret_cast<base::RefCountedStringImpl*>(value.Ptr());
  int32_t status = LYNX_CAPI_MODULE_SUCCESS;
  state_.handler = [&](lynx_value* result) {
    *result = value.value();
    lynx_value_add_reference(nullptr, *result, nullptr);
    return status;
  };
  auto response = module_->InvokeMethod("call", CreateArguments(), 0, {});
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(response.value()->str(), "result");
  EXPECT_FALSE(string->HasOneRef());
  response.value().reset();
  EXPECT_TRUE(string->HasOneRef());

  status = LYNX_CAPI_MODULE_ERROR;
  EXPECT_FALSE(
      module_->InvokeMethod("call", CreateArguments(), 0, {}).has_value());
  EXPECT_TRUE(string->HasOneRef());
}

TEST_F(LynxCapiModuleTest, RejectsExtendedReturnWithoutDereferencingIt) {
  Register();
  state_.handler = [](lynx_value* result) {
    result->type = lynx_value_extended;
    return LYNX_CAPI_MODULE_SUCCESS;
  };
  EXPECT_FALSE(
      module_->InvokeMethod("call", CreateArguments(), 0, {}).has_value());
}

TEST_F(LynxCapiModuleTest, InvokesRetainedCallbackOnlyOnce) {
  CreateCallback();
  lynx_value extended{};
  extended.type = lynx_value_extended;
  EXPECT_EQ(lynx_native_module_callback_invoke(handle(), &extended, 1), 0);
  lepus::Value value("callback");
  auto argument = value.value();
  EXPECT_EQ(lynx_native_module_callback_invoke(handle(), &argument, 1), 1);
  EXPECT_EQ(lynx_native_module_callback_invoke(handle(), &argument, 1), 0);
  EXPECT_EQ(delegate_->invoke_count, 1);
  ASSERT_NE(callback_->args(), nullptr);
  EXPECT_EQ(callback_->args()->GetValueAtIndex(0)->str(), "callback");
}

TEST_F(LynxCapiModuleTest, RetainsCallbackArgumentsUntilExecution) {
  CreateCallback();
  delegate_->defer_callbacks = true;
  {
    auto bytes = std::make_unique<uint8_t[]>(1);
    bytes[0] = 42;
    auto array = lepus::CArray::Create();
    array->emplace_back("retained");
    array->emplace_back(lepus::ByteArray::Create(std::move(bytes), 1));
    lepus::Value value(array);
    auto argument = value.value();
    EXPECT_EQ(lynx_native_module_callback_invoke(handle(), &argument, 1), 1);
    // Updating a retained container before the task runs remains visible,
    // just like Android's GlobalRef-backed callback arguments.
    array->set(0, lepus::Value("updated"));
  }
  delegate_->FlushCallbacks();
  ASSERT_NE(callback_->args(), nullptr);
  auto argument = callback_->args()->GetValueAtIndex(0);
  EXPECT_EQ(argument->GetValueAtIndex(0)->str(), "updated");
  auto buffer = argument->GetValueAtIndex(1);
  ASSERT_TRUE(buffer->IsArrayBuffer());
  EXPECT_EQ(buffer->ArrayBuffer()[0], 42);
}

TEST_F(LynxCapiModuleTest, InvocationErrorDoesNotCancelCallback) {
  Register(1);
  delegate_->defer_callbacks = true;
  state_.handler = [](lynx_value*) { return LYNX_CAPI_MODULE_ERROR; };
  EXPECT_FALSE(module_
                   ->InvokeMethod("call",
                                  CreateArguments({lepus::Value(int64_t{1})}),
                                  1, {{0, callback_}})
                   .has_value());
  EXPECT_EQ(lynx_native_module_callback_invoke(handle(), nullptr, 0), 1);
  delegate_->FlushCallbacks();
  EXPECT_EQ(delegate_->invoke_count, 1);
  ASSERT_NE(callback_->args(), nullptr);
  EXPECT_EQ(callback_->args()->Length(), 0);
  EXPECT_EQ(lynx_native_module_callback_invoke(handle(), nullptr, 0), 0);
}

TEST_F(LynxCapiModuleTest, PreservesCallbackArgumentIndices) {
  Register(3);
  auto response = module_->InvokeMethod(
      "call",
      CreateArguments({lepus::Value(int64_t{1}), lepus::Value(2),
                       lepus::Value(int64_t{3})}),
      3, {{2, std::make_shared<TestModuleCallback>()}, {0, callback_}});
  ASSERT_TRUE(response.has_value());
  ASSERT_EQ(state_.callbacks.size(), 2u);
  for (const auto& descriptor : state_.callbacks) {
    if (descriptor.argument_index == 0) {
      EXPECT_EQ(
          lynx_native_module_callback_invoke(descriptor.callback, nullptr, 0),
          1);
      EXPECT_EQ(delegate_->last_callback, callback_);
    } else {
      EXPECT_EQ(descriptor.argument_index, 2u);
    }
  }
}

TEST_F(LynxCapiModuleTest, CancelsQueuedAndRetainedCallbacksOnDetach) {
  CreateCallback();
  ASSERT_TRUE(module_
                  ->InvokeMethod("call", CreateArguments({lepus::Value(1)}), 1,
                                 {{0, std::make_shared<TestModuleCallback>()}})
                  .has_value());
  ASSERT_EQ(state_.callbacks.size(), 2u);
  delegate_->defer_callbacks = true;
  EXPECT_EQ(lynx_native_module_callback_invoke(handle(), nullptr, 0), 1);
  registry_->Detach();
  delegate_->FlushCallbacks();
  EXPECT_EQ(delegate_->invoke_count, 0);
  EXPECT_EQ(callback_->args(), nullptr);
  EXPECT_FALSE(
      module_->InvokeMethod("call", CreateArguments({lepus::Value(1)}), 1, {})
          .has_value());
  EXPECT_EQ(lynx_native_module_callback_invoke(state_.callbacks[1].callback,
                                               nullptr, 0),
            0);
  EXPECT_EQ(LynxCapiModuleFactory(registry_).CreateModule("TestModule"),
            nullptr);
}

TEST_F(LynxCapiModuleTest, DestroyCancelsOnlyItsOwnRuntimeModule) {
  CreateCallback();
  auto other = LynxCapiModuleFactory(registry_).CreateModule("TestModule");
  other->SetDelegate(delegate_);
  delegate_->defer_callbacks = true;
  ASSERT_EQ(lynx_native_module_callback_invoke(handle(), nullptr, 0), 1);
  module_->Destroy();
  delegate_->FlushCallbacks();
  EXPECT_EQ(delegate_->invoke_count, 0);
  EXPECT_FALSE(
      module_->InvokeMethod("call", CreateArguments({lepus::Value(1)}), 1, {})
          .has_value());
  EXPECT_TRUE(
      other->InvokeMethod("call", CreateArguments({lepus::Value(1)}), 1, {})
          .has_value());
}

TEST_F(LynxCapiModuleTest, ConcurrentCallbackCallsAcceptOnlyOne) {
  CreateCallback();
  std::atomic<int> accepted{0};
  std::vector<std::thread> threads;
  for (int index = 0; index < 8; ++index) {
    threads.emplace_back([&] {
      accepted.fetch_add(
          lynx_native_module_callback_invoke(handle(), nullptr, 0));
    });
  }
  for (auto& thread : threads) {
    thread.join();
  }
  EXPECT_EQ(accepted.load(), 1);
  EXPECT_EQ(delegate_->invoke_count, 1);
}

TEST(LynxCapiModuleDelegateTest, LepusChecksPreconditionOnEngineThread) {
  class TaskRunner final : public fml::TaskRunner {
   public:
    TaskRunner() : fml::TaskRunner(nullptr) {}
    bool RunsTasksOnCurrentThread() override { return false; }
    void PostTask(base::closure task) override { pending = std::move(task); }
    base::closure pending;
  };
  auto runner = fml::AdoptRef(new TaskRunner());
  auto actor = std::make_shared<shell::LynxActor<shell::LynxEngine>>(
      std::make_unique<shell::LynxEngine>(nullptr, nullptr, nullptr,
                                          base::LogContext{}, -1),
      runner);
  lepus::LepusModuleDelegate delegate;
  delegate.SetEngineActor(actor);
  bool checked = false;
  delegate.InvokeCallback(std::make_shared<TestModuleCallback>(), [&] {
    checked = true;
    return false;
  });
  EXPECT_FALSE(checked);
  ASSERT_TRUE(runner->pending);
  runner->pending();
  EXPECT_TRUE(checked);
}

class LynxCapiModuleLepusNGTest : public LynxCapiModuleTest,
                                  public testing::WithParamInterface<bool> {};

TEST_P(LynxCapiModuleLepusNGTest,
       MaterializesArrayBuffersWithoutChangingSource) {
  lepus::QuickContext context(nullptr, GetParam());
  auto* ctx = context.context();
  lepus::LepusValueFactory factory(ctx);
  const uint8_t bytes[] = {0, 127, 255};
  auto buffer =
      factory.Create(LEPUS_NewArrayBufferCopy(ctx, bytes, sizeof(bytes)));
  auto nested = factory.Create(LEPUS_NewArray(ctx));
  nested.SetProperty(0, buffer);
  auto object = factory.Create(LEPUS_NewObject(ctx));
  object.SetProperty("items", nested);
  auto empty = factory.Create(LEPUS_NewArrayBufferCopy(ctx, bytes, 0));
  Register(3);

  auto response = module_->InvokeMethod(
      "call", CreateArguments({buffer, object, empty}), 3, {});
  ASSERT_TRUE(response.has_value());
  ASSERT_EQ(state_.arguments.size(), 3u);
  auto nested_buffer = state_.arguments[1].GetProperty("items").GetProperty(0);
  for (const auto& value : {state_.arguments[0], nested_buffer}) {
    ASSERT_TRUE(value.IsByteArray());
    ASSERT_EQ(value.ByteArray()->GetLength(), sizeof(bytes));
    EXPECT_EQ(value.ByteArray()->GetPtr()[0], 0);
    EXPECT_EQ(value.ByteArray()->GetPtr()[1], 127);
    EXPECT_EQ(value.ByteArray()->GetPtr()[2], 255);
  }
  ASSERT_TRUE(state_.arguments[2].IsByteArray());
  EXPECT_EQ(state_.arguments[2].ByteArray()->GetLength(), 0u);
  state_.arguments[0].ByteArray()->GetPtr()[1] = 42;
  EXPECT_TRUE(buffer.IsJSValue());
  EXPECT_TRUE(object.IsJSValue());
  auto original_buffer = object.GetProperty("items").GetProperty(0);
  original_buffer.ToLepusValue();
  ASSERT_TRUE(original_buffer.IsByteArray());
  EXPECT_EQ(original_buffer.ByteArray()->GetPtr()[1], 127);
}

TEST_P(LynxCapiModuleLepusNGTest,
       ReturnsArrayBuffersAndNaNToJSWithoutChangingSource) {
  lepus::QuickContext context(nullptr, GetParam());
  auto* ctx = context.context();
  lepus::LepusValueFactory factory(ctx);
  const uint8_t bytes[] = {0, 127, 255};
  auto input =
      factory.Create(LEPUS_NewArrayBufferCopy(ctx, bytes, sizeof(bytes)));
  Register(1);
  state_.handler = [&](lynx_value* result) {
    *result = state_.arguments[0].value();
    lynx_value_add_reference(nullptr, *result, nullptr);
    return LYNX_CAPI_MODULE_SUCCESS;
  };
  auto response =
      module_->InvokeMethod("call", CreateArguments({input}), 1, {});
  ASSERT_TRUE(response.has_value());
  auto source = pub::ValueUtils::ConvertValueToLepusValue(*response.value());
  ASSERT_TRUE(source.IsByteArray());
  auto* original = source.ByteArray()->GetPtr();
  auto items = lepus::CArray::Create();
  items->push_back(source);
  auto object = lepus::Dictionary::Create();
  object->SetValue("items", lepus::Value(items));
  object->SetValue("empty", lepus::Value(lepus::ByteArray::Create()));
  lynx_value nan{};
  nan.type = lynx_value_nan;
  object->SetValue("nan", lepus::Value(nullptr, nan));
  lepus::Value nested(object);
  const char script[] = R"(
    (function(buffer, nested) {
      const copy = nested.items[0];
      const a = new Uint8Array(buffer);
      const b = new Uint8Array(copy);
      const valid = buffer instanceof ArrayBuffer && buffer.byteLength === 3 &&
        copy instanceof ArrayBuffer && copy.byteLength === 3 &&
        nested.empty instanceof ArrayBuffer && nested.empty.byteLength === 0 &&
        Number.isNaN(nested.nan) &&
        a[0] === 0 && a[1] === 127 && a[2] === 255 &&
        b[0] === 0 && b[1] === 127 && b[2] === 255;
      a[1] = 42;
      return valid && b[1] === 127;
    })
  )";
  auto check =
      factory.Create(LEPUS_Eval(ctx, script, sizeof(script) - 1,
                                "capi-buffer-test.js", LEPUS_EVAL_TYPE_GLOBAL));
  for (bool deep_convert : {false, true}) {
    auto converted = factory.Create(
        lepus::LEPUSValueHelper::ToJsValue(ctx, source, deep_convert));
    auto converted_nested = factory.Create(
        lepus::LEPUSValueHelper::ToJsValue(ctx, nested, deep_convert));
    const lepus::Value* args[] = {&converted, &converted_nested};
    EXPECT_TRUE(context.CallClosureArgs(check, args, 2).Bool());
    ASSERT_EQ(source.ByteArray()->GetPtr(), original);
    EXPECT_EQ(source.ByteArray()->GetLength(), sizeof(bytes));
    EXPECT_EQ(source.ByteArray()->GetPtr()[1], 127);
  }
  input.ToLepusValue();
  EXPECT_EQ(input.ByteArray()->GetPtr()[1], 127);
}

TEST_P(LynxCapiModuleLepusNGTest, MaterializesScalarsAndNestedContainers) {
  lepus::QuickContext context(nullptr, GetParam());
  auto* ctx = context.context();
  lepus::LepusValueFactory factory(ctx);
  Register(5);
  auto nested = LEPUS_NewArray(ctx);
  LEPUS_SetPropertyUint32(ctx, nested, 0, LEPUS_NewString(ctx, "nested"));
  auto object = LEPUS_NewObject(ctx);
  LEPUS_SetPropertyStr(ctx, object, "items", nested);
  auto array = lepus::CArray::Create();
  array->push_back(factory.Create(LEPUS_NewString(ctx, "hello")));
  array->push_back(factory.Create(LEPUS_NewInt32(ctx, 42)));
  array->push_back(factory.Create(LEPUS_NewBool(ctx, true)));
  array->push_back(factory.Create(std::move(object)));
  array->push_back(factory.Create(LEPUS_NewArray(ctx)));
  for (size_t index = 0; index < array->size(); ++index) {
    ASSERT_EQ(array->get(index).value().type, lynx_value_extended);
  }

  auto response = module_->InvokeMethod(
      "call", std::make_unique<pub::ValueImplLepus>(lepus::Value(array)), 5,
      {});
  ASSERT_TRUE(response.has_value());
  ASSERT_EQ(state_.arguments.size(), 5u);
  EXPECT_EQ(state_.arguments[0].value().type, lynx_value_string);
  EXPECT_EQ(state_.arguments[0].StdString(), "hello");
  EXPECT_EQ(state_.arguments[1].value().type, lynx_value_int32);
  EXPECT_EQ(state_.arguments[1].Int32(), 42);
  EXPECT_EQ(state_.arguments[2].value().type, lynx_value_bool);
  EXPECT_TRUE(state_.arguments[2].Bool());
  EXPECT_EQ(state_.arguments[3].value().type, lynx_value_map);
  auto items = state_.arguments[3].GetProperty(base::String("items"));
  ASSERT_EQ(items.value().type, lynx_value_array);
  EXPECT_EQ(items.Array()->get(0).value().type, lynx_value_string);
  EXPECT_EQ(items.Array()->get(0).StdString(), "nested");
  EXPECT_EQ(state_.arguments[4].value().type, lynx_value_array);
}

TEST_P(LynxCapiModuleLepusNGTest, RejectsNestedJSFunctions) {
  lepus::QuickContext context(nullptr, GetParam());
  auto* ctx = context.context();
  lepus::LepusValueFactory factory(ctx);
  auto callback = factory.Create(LEPUS_NewCFunction(
      ctx,
      [](LEPUSContext*, LEPUSValueConst, int, LEPUSValueConst*) {
        return LEPUS_UNDEFINED;
      },
      "callback", 0));
  auto object = factory.Create(LEPUS_NewObject(ctx));
  object.SetProperty("callback", callback);
  Register(1);
  EXPECT_FALSE(module_->InvokeMethod("call", CreateArguments({object}), 1, {})
                   .has_value());
  EXPECT_EQ(state_.invoke_count, 0);
}

INSTANTIATE_TEST_SUITE_P(GCAndRefCounted, LynxCapiModuleLepusNGTest,
                         testing::Bool());

int32_t InvokeEmpty(void*, const char*, const lynx_value*, size_t,
                    const lynx_capi_module_callback_t*, size_t, lynx_value*) {
  return LYNX_CAPI_MODULE_SUCCESS;
}

void CountFinalizer(void* opaque) { ++*static_cast<int*>(opaque); }

TEST(LynxCapiModuleRegistrationTest,
     CopiesMetadataAndFinalizesReplacedBuilderEntry) {
  int finalized = 0;
  {
    lynx_view_builder_t builder;
    char module_name[] = "TestModule";
    char method_name[] = "call";
    lynx_capi_module_method_t method{method_name, 0};
    ASSERT_EQ(lynx_view_builder_register_capi_module(
                  &builder, module_name, &method, 1, InvokeEmpty, &finalized,
                  CountFinalizer),
              LYNX_CAPI_MODULE_SUCCESS);
    module_name[0] = 'X';
    method_name[0] = 'X';
    EXPECT_EQ(builder.capi_modules.at("TestModule")->methods.count("call"), 1u);
    method.name = "replacement";
    ASSERT_EQ(lynx_view_builder_register_capi_module(
                  &builder, "TestModule", &method, 1, InvokeEmpty, &finalized,
                  CountFinalizer),
              LYNX_CAPI_MODULE_SUCCESS);
    EXPECT_EQ(finalized, 1);
  }
  EXPECT_EQ(finalized, 2);
}

TEST(LynxCapiModuleRegistrationTest,
     RejectsInvalidRegistrationWithoutTakingOwnership) {
  lynx_view_builder_t builder;
  int finalized = 0;
  lynx_capi_module_method_t methods[] = {{"call", 0}, {"call", 1}};
  auto register_module = [&](lynx_view_builder_t* target, const char* name,
                             size_t count, lynx_capi_module_invoke invoke) {
    return lynx_view_builder_register_capi_module(
        target, name, methods, count, invoke, &finalized, CountFinalizer);
  };
  EXPECT_EQ(register_module(nullptr, "Module", 1, InvokeEmpty),
            LYNX_CAPI_MODULE_ERROR);
  EXPECT_EQ(register_module(&builder, "", 1, InvokeEmpty),
            LYNX_CAPI_MODULE_ERROR);
  EXPECT_EQ(register_module(&builder, "Module", 0, InvokeEmpty),
            LYNX_CAPI_MODULE_ERROR);
  EXPECT_EQ(register_module(&builder, "Module", 1, nullptr),
            LYNX_CAPI_MODULE_ERROR);
  EXPECT_EQ(register_module(&builder, "Module", 2, InvokeEmpty),
            LYNX_CAPI_MODULE_ERROR);
  methods[0].argument_count = std::numeric_limits<size_t>::max();
  EXPECT_EQ(register_module(&builder, "Module", 1, InvokeEmpty),
            LYNX_CAPI_MODULE_ERROR);
  EXPECT_TRUE(builder.capi_modules.empty());
  EXPECT_EQ(finalized, 0);
}

TEST(LynxCapiModuleRegistrationTest, FinalizesOnceOutsideRegistryLock) {
  lynx_view_builder_t builder;
  struct State {
    int finalized = 0;
    std::weak_ptr<LynxCapiModuleRegistry> registry;
  } state;
  const lynx_capi_module_method_t method{"call", 0};
  ASSERT_EQ(lynx_view_builder_register_capi_module(
                &builder, "Module", &method, 1, InvokeEmpty, &state,
                [](void* opaque) {
                  auto& state = *static_cast<State*>(opaque);
                  ++state.finalized;
                  if (auto registry = state.registry.lock()) {
                    EXPECT_FALSE(registry->IsActive());
                    EXPECT_EQ(registry->GetModule("Module"), nullptr);
                  }
                }),
            LYNX_CAPI_MODULE_SUCCESS);
  auto registry =
      std::make_shared<LynxCapiModuleRegistry>(std::move(builder.capi_modules));
  state.registry = registry;
  registry->Detach();
  registry->Detach();
  EXPECT_EQ(state.finalized, 1);
}

TEST(LynxCapiModuleRegistrationTest,
     ConcurrentDetachKeepsOpaqueAliveWithoutBlocking) {
  struct State {
    std::mutex mutex;
    std::condition_variable condition;
    bool entered = false;
    bool finish = false;
    std::atomic<int> finalized{0};
  } state;
  lynx_view_builder_t builder;
  const lynx_capi_module_method_t method{"call", 0};
  ASSERT_EQ(lynx_view_builder_register_capi_module(
                &builder, "Module", &method, 1,
                [](void* opaque, const char*, const lynx_value*, size_t,
                   const lynx_capi_module_callback_t*, size_t,
                   lynx_value*) -> int32_t {
                  auto& state = *static_cast<State*>(opaque);
                  std::unique_lock<std::mutex> lock(state.mutex);
                  state.entered = true;
                  state.condition.notify_all();
                  state.condition.wait(lock, [&] { return state.finish; });
                  EXPECT_EQ(state.finalized.load(), 0);
                  return LYNX_CAPI_MODULE_SUCCESS;
                },
                &state,
                [](void* opaque) {
                  static_cast<State*>(opaque)->finalized.fetch_add(1);
                }),
            LYNX_CAPI_MODULE_SUCCESS);
  auto registry =
      std::make_shared<LynxCapiModuleRegistry>(std::move(builder.capi_modules));
  auto module = LynxCapiModuleFactory(registry).CreateModule("Module");
  auto delegate = std::make_shared<TestModuleDelegate>();
  module->SetDelegate(delegate);
  std::thread invoker([&] {
    EXPECT_TRUE(
        module->InvokeMethod("call", CreateArguments(), 0, {}).has_value());
  });
  {
    std::unique_lock<std::mutex> lock(state.mutex);
    EXPECT_TRUE(state.condition.wait_for(lock, std::chrono::seconds(5),
                                         [&] { return state.entered; }));
  }
  registry->Detach();
  EXPECT_EQ(state.finalized.load(), 0);
  EXPECT_FALSE(
      module->InvokeMethod("call", CreateArguments(), 0, {}).has_value());
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.finish = true;
  }
  state.condition.notify_all();
  invoker.join();
  EXPECT_EQ(state.finalized.load(), 1);
}

}  // namespace
}  // namespace embedder
}  // namespace lynx
