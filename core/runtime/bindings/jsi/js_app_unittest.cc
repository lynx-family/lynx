// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/js_app.h"

#include <memory>
#include <optional>
#include <type_traits>

#include "base/include/debug/lynx_error.h"
#include "base/include/fml/message_loop.h"
#include "base/include/to_underlying.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/bindings/jsi/event/context_proxy_in_js.h"
#include "core/runtime/bindings/jsi/js_object_destruction_observer.h"
#include "core/runtime/bindings/jsi/lynx.h"
#include "core/runtime/bindings/jsi/mock_module_delegate.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_impl.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_manager.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/jsi_unittest.h"
#include "core/runtime/piper/js/js_bundle.h"
#include "core/runtime/piper/js/lynx_api_handler.h"
#include "core/runtime/piper/js/mock_template_delegate.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "lynx/core/runtime/bindings/jsi/modules/module_delegate.h"
#include "lynx_sub_error_code.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace test {

constexpr int ERROR_CODE_SIZE = 7;
const int ERROR_CODE[ERROR_CODE_SIZE] = {
    error::E_SUCCESS,
    error::E_NATIVE_MODULES_COMMON_WRONG_PARAM_TYPE,
    error::E_NATIVE_MODULES_COMMON_WRONG_PARAM_NUM,
    error::E_NATIVE_MODULES_COMMON_SYSTEM_AUTHORIZATION_ERROR,
    error::E_NATIVE_MODULES_COMMON_AUTHORIZATION_ERROR,
    error::E_NATIVE_MODULES_COMMON_RETURN_ERROR,
    error::E_NATIVE_MODULES_EXCEPTION};

class MockVSyncObserver : public runtime::IVSyncObserver {
 public:
  void RequestAnimationFrame(
      uintptr_t id, base::MoveOnlyClosure<void, int64_t, int64_t> callback) {}

  void RequestBeforeAnimationFrame(
      uintptr_t id, base::MoveOnlyClosure<void, int64_t, int64_t> callback) {}

  void RegisterAfterAnimationFrameListener(
      base::MoveOnlyClosure<void, int64_t, int64_t> callback) {}
};

class MockDelegate : public runtime::test::MockTemplateDelegate {
 public:
  void SetCacheDataOP(std::vector<shell::CacheDataOp> op) {
    cache_data_op_ = std::move(op);
  }

  std::vector<shell::CacheDataOp> FetchUpdatedCardData() override {
    return std::move(cache_data_op_);
  }

  MOCK_METHOD(piper::JsContent, GetJSContentFromExternal,
              (const std::string&, const std::string&, long), (override));

  std::shared_ptr<runtime::IVSyncObserver> GetVSyncObserver() override {
    return std::make_shared<MockVSyncObserver>();
  }

  MOCK_METHOD(void, OnErrorOccurred, (base::LynxError), (override));

 private:
  std::vector<shell::CacheDataOp> cache_data_op_;
};

class MockErrorNativeModule : public LynxNativeModule {
 public:
  MockErrorNativeModule() = default;
  ~MockErrorNativeModule() override = default;

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const CallbackMap& callbacks) override {
    return base::unexpected<std::string>("error");
  }
};

class MockTestNativeModule : public LynxNativeModule {
 public:
  MockTestNativeModule() : LynxNativeModule() {
    NativeModuleMethod get_string("getString", 1);
    RegisterMethod(get_string, reinterpret_cast<NativeModuleInvocation>(
                                   &MockTestNativeModule::GetString));

    NativeModuleMethod get_number("getNumber", 1);
    RegisterMethod(get_number, reinterpret_cast<NativeModuleInvocation>(
                                   &MockTestNativeModule::GetNumber));

    NativeModuleMethod get_bool("getBool", 1);
    RegisterMethod(get_bool, reinterpret_cast<NativeModuleInvocation>(
                                 &MockTestNativeModule::GetBool));

    NativeModuleMethod get_array("getArray", 1);
    RegisterMethod(get_array, reinterpret_cast<NativeModuleInvocation>(
                                  &MockTestNativeModule::GetArray));

    NativeModuleMethod get_object("getObject", 1);
    RegisterMethod(get_object, reinterpret_cast<NativeModuleInvocation>(
                                   &MockTestNativeModule::GetObject));

    NativeModuleMethod sync_callback("syncCallback", 1);
    RegisterMethod(sync_callback, reinterpret_cast<NativeModuleInvocation>(
                                      &MockTestNativeModule::SyncCallback));

    NativeModuleMethod foo("sameName", 0);
    RegisterMethod(foo, reinterpret_cast<NativeModuleInvocation>(
                            &MockTestNativeModule::Foo));

    NativeModuleMethod try_error("tryError", 0);
    RegisterMethod(try_error, reinterpret_cast<NativeModuleInvocation>(
                                  &MockTestNativeModule::TryError));
  }

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const CallbackMap& callbacks) override {
    auto invocation_itr = invocations_.find(method_name);
    if (invocation_itr != invocations_.end()) {
      return (this->*(invocation_itr->second))(std::move(args), callbacks);
    }
    return std::unique_ptr<pub::Value>(nullptr);
  }

  std::unique_ptr<pub::Value> GetString(std::unique_ptr<pub::Value> args,
                                        const CallbackMap& callbacks) {
    EXPECT_TRUE(args->IsArray());
    EXPECT_EQ(args->Length(), 1);
    auto arg = args->GetValueAtIndex(0);
    EXPECT_TRUE(arg->IsString());
    return arg;
  }

  std::unique_ptr<pub::Value> GetNumber(std::unique_ptr<pub::Value> args,
                                        const CallbackMap& callbacks) {
    EXPECT_TRUE(args->IsArray());
    EXPECT_EQ(args->Length(), 1);
    auto arg = args->GetValueAtIndex(0);
    EXPECT_TRUE(arg->IsNumber());
    return arg;
  }

  std::unique_ptr<pub::Value> GetBool(std::unique_ptr<pub::Value> args,
                                      const CallbackMap& callbacks) {
    EXPECT_TRUE(args->IsArray());
    EXPECT_EQ(args->Length(), 1);
    auto arg = args->GetValueAtIndex(0);
    EXPECT_TRUE(arg->IsBool());
    return arg;
  }

  std::unique_ptr<pub::Value> GetArray(std::unique_ptr<pub::Value> args,
                                       const CallbackMap& callbacks) {
    EXPECT_TRUE(args->IsArray());
    EXPECT_EQ(args->Length(), 1);
    auto arg = args->GetValueAtIndex(0);
    EXPECT_TRUE(arg->IsArray());
    return arg;
  }

  std::unique_ptr<pub::Value> GetObject(std::unique_ptr<pub::Value> args,
                                        const CallbackMap& callbacks) {
    EXPECT_TRUE(args->IsArray());
    EXPECT_EQ(args->Length(), 1);
    auto arg = args->GetValueAtIndex(0);
    EXPECT_TRUE(arg->IsMap());
    return arg;
  }

  std::unique_ptr<pub::Value> SyncCallback(std::unique_ptr<pub::Value> args,
                                           const CallbackMap& callbacks) {
    EXPECT_TRUE(args->IsArray());
    EXPECT_EQ(args->Length(), 1);
    auto arg = args->GetValueAtIndex(0);
    EXPECT_TRUE(arg->IsInt64());
    auto it = callbacks.find(0);
    EXPECT_NE(it, callbacks.end());
    auto delegate = delegate_.lock();
    if (delegate) {
      delegate->InvokeCallback(it->second);
    }
    return std::unique_ptr<pub::Value>();
  }

  std::unique_ptr<pub::Value> Foo(std::unique_ptr<pub::Value> args,
                                  const CallbackMap& callbacks) {
    auto delegate = delegate_.lock();
    EXPECT_NE(delegate, nullptr);
    return delegate->GetValueFactory()->CreateString("native_module");
  }

  std::unique_ptr<pub::Value> TryError(std::unique_ptr<pub::Value> args) {
    auto delegate = delegate_.lock();
    EXPECT_NE(delegate, nullptr);
    EXPECT_TRUE(args->GetValueAtIndex(0)->IsNumber());
    auto error_code = static_cast<int>(args->GetValueAtIndex(0)->Number());
    if (error_code != 0) {
      std::string module_name = "ut_module";
      std::string method_name = "tryError";
      EXPECT_LE(error_code, ERROR_CODE_SIZE);
      int module_error_code = ERROR_CODE[error_code];
      delegate->OnErrorOccurred(module_name, method_name,
                                base::LynxError{module_error_code, ""});
    }
    return std::unique_ptr<pub::Value>();
  }

 protected:
  void RegisterMethod(const NativeModuleMethod& method,
                      NativeModuleInvocation invocation) {
    methods_.emplace(method.name, method);
    invocations_.emplace(method.name, std::move(invocation));
  }

 private:
  std::unordered_map<std::string, NativeModuleInvocation> invocations_;
};

class MockPlatformModuleFactory : public NativeModuleFactory {
 public:
  MockPlatformModuleFactory(std::shared_ptr<ModuleDelegate> delegate)
      : delegate_(std::move(delegate)) {}
  virtual ~MockPlatformModuleFactory() = default;

  virtual std::shared_ptr<LynxModule> CreatePlatformModule(
      const std::string& name) {
    auto itr = creators_.find(name);
    if (itr == creators_.end()) {
      return nullptr;
    }
    auto native_module = itr->second();
    auto lynx_module =
        std::make_shared<LynxModuleImpl>(name, delegate_, native_module);
    return lynx_module;
  };

  virtual void Register(const std::string& name, ModuleCreator creator) {
    creators_.emplace(name, std::move(creator));
  }

 private:
  std::unordered_map<std::string, ModuleCreator> creators_;
  std::shared_ptr<ModuleDelegate> delegate_;
};

class MockTestPlatformModule : public LynxNativeModule {
 public:
  MockTestPlatformModule() : LynxNativeModule() {
    NativeModuleMethod foo("sameName", 0);
    RegisterMethod(foo, reinterpret_cast<NativeModuleInvocation>(
                            &MockTestPlatformModule::Foo));

    NativeModuleMethod foo2("notSameName", 0);
    RegisterMethod(foo2, reinterpret_cast<NativeModuleInvocation>(
                             &MockTestPlatformModule::FooLowLevel));
  }

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const CallbackMap& callbacks) override {
    auto invocation_itr = invocations_.find(method_name);
    if (invocation_itr != invocations_.end()) {
      return (this->*(invocation_itr->second))(std::move(args), callbacks);
    }
    return std::unique_ptr<pub::Value>(nullptr);
  }

  std::unique_ptr<pub::Value> Foo(std::unique_ptr<pub::Value> args,
                                  const CallbackMap& callbacks) {
    auto delegate = delegate_.lock();
    EXPECT_NE(delegate, nullptr);
    return delegate->GetValueFactory()->CreateString("platform_module");
  }

  std::unique_ptr<pub::Value> FooLowLevel(std::unique_ptr<pub::Value> args,
                                          const CallbackMap& callbacks) {
    auto delegate = delegate_.lock();
    EXPECT_NE(delegate, nullptr);
    return delegate->GetValueFactory()->CreateString("platform_module");
  }

 protected:
  void RegisterMethod(const NativeModuleMethod& method,
                      NativeModuleInvocation invocation) {
    methods_.emplace(method.name, method);
    invocations_.emplace(method.name, std::move(invocation));
  }

 private:
  std::unordered_map<std::string, NativeModuleInvocation> invocations_;
};

class MockJsApp : public HostObject {
 public:
  MockJsApp(std::weak_ptr<Runtime> rt) : rt_(rt) {}
  ~MockJsApp() = default;

  virtual Value get(Runtime*, const PropNameID& name) override;
  virtual void set(Runtime*, const PropNameID& name,
                   const Value& value) override;
  virtual std::vector<PropNameID> getPropertyNames(Runtime& rt) override;

  size_t call_count{0};
  std::vector<size_t> count_ary{0};
  std::vector<std::vector<piper::Value>> args_ary;

 private:
  std::weak_ptr<Runtime> rt_;
};

Value MockJsApp::get(lynx::piper::Runtime* rt,
                     const lynx::piper::PropNameID& name) {
  auto methodName = name.utf8(*rt);
  if (rt == nullptr) {
    return piper::Value::undefined();
  }
  return Function::createFromHostFunction(
      *rt, PropNameID::forAscii(*rt, "allFunctions"), 1,
      [this](Runtime& rt, const Value& thisVal, const Value* args,
             size_t count) {
        this->call_count++;
        this->count_ary.emplace_back(count);
        std::vector<piper::Value> temp_args;
        for (size_t i = 0; i < count; ++i) {
          temp_args.emplace_back(piper::Value(rt, *(args + i)));
        }
        this->args_ary.emplace_back(std::move(temp_args));
        return piper::Value::undefined();
      });
}

void MockJsApp::set(Runtime*, const PropNameID& name, const Value& value) {}

std::vector<PropNameID> MockJsApp::getPropertyNames(Runtime& rt) {
  std::vector<PropNameID> vec;
  vec.push_back(piper::PropNameID::forUtf8(rt, "loadCard"));
  return vec;
}

class AppTest : public JSITestBase {
 public:
  AppTest() : mock_js_app_(std::make_shared<MockJsApp>(runtime)) {}

  std::shared_ptr<App> app;

 protected:
  void SetUp() override {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto nativeModule =
        eval("(function() { return {}; })()")->asObject(rt).value();
    InitModuleManager();
    piper::Object nativeModuleProxy = piper::Object::createFromHostObject(
        *runtime, module_manager_.get()->bindingPtr);

    app = App::Create(0, runtime, &delegate_, exception_handler_,
                      std::move(nativeModuleProxy), nullptr, "-1");

    app->setJsAppObj(Object::createFromHostObject(*runtime, mock_js_app_));
  }

  void InitModuleManager() {
    module_manager_ = std::make_shared<LynxModuleManager>();
    module_delegate_ = std::make_shared<MockModuleDelegate>(&rt);
    module_manager_->initBindingPtr(module_manager_, module_delegate_);

    auto platform_module_factory =
        std::make_unique<MockPlatformModuleFactory>(module_delegate_);
    platform_module_factory->Register("ut_module", []() {
      return std::make_shared<MockTestPlatformModule>();
    });
    module_manager_->SetPlatformModuleFactory(
        std::move(platform_module_factory));

    auto native_module_factory = std::make_unique<NativeModuleFactory>();
    native_module_factory->Register(
        "ut_module", []() { return std::make_shared<MockTestNativeModule>(); });
    module_manager_->SetModuleFactory(std::move(native_module_factory));
    module_manager_->SetRecordID(12345);
  }

  std::shared_ptr<MockJsApp> mock_js_app_;
  ::testing::StrictMock<MockDelegate> delegate_;
  std::shared_ptr<LynxModuleManager> module_manager_;
  std::shared_ptr<MockModuleDelegate> module_delegate_;
};

TEST_P(AppTest, CreateAppTest) { EXPECT_TRUE(app); }

TEST_P(AppTest, NativeLynxContextProxyTest) {
  EXPECT_TRUE(app);

  auto lynx_proxy = std::make_shared<piper::LynxProxy>(runtime, app);
  Object obj = Object::createFromHostObject(rt, lynx_proxy);

  auto res1 =
      function("function(lynx) { return lynx.getDevtool(); }").call(rt, obj);
  EXPECT_TRUE(res1->isObject());
  auto proxy = app->GetContextProxy(runtime::ContextProxy::Type::kDevTool);
  EXPECT_TRUE(proxy != nullptr);
  EXPECT_EQ(proxy->GetTargetType(), runtime::ContextProxy::Type::kDevTool);

  auto res2 =
      function("function(lynx) { return lynx.getJSContext(); }").call(rt, obj);
  EXPECT_TRUE(res2->isObject());
  proxy = app->GetContextProxy(runtime::ContextProxy::Type::kJSContext);
  EXPECT_TRUE(proxy != nullptr);
  EXPECT_EQ(proxy->GetTargetType(), runtime::ContextProxy::Type::kJSContext);

  auto res3 = function("function(lynx) { return lynx.getCoreContext(); }")
                  .call(rt, obj);
  EXPECT_TRUE(res3->isObject());
  proxy = app->GetContextProxy(runtime::ContextProxy::Type::kCoreContext);
  EXPECT_TRUE(proxy != nullptr);
  EXPECT_EQ(proxy->GetTargetType(), runtime::ContextProxy::Type::kCoreContext);

  auto res4 =
      function("function(lynx) { return lynx.getUIContext(); }").call(rt, obj);
  EXPECT_TRUE(res4->isObject());
  proxy = app->GetContextProxy(runtime::ContextProxy::Type::kUIContext);
  EXPECT_TRUE(proxy != nullptr);
  EXPECT_EQ(proxy->GetTargetType(), runtime::ContextProxy::Type::kUIContext);
}

TEST_P(AppTest, LoadAppTest) {
  EXPECT_TRUE(app);

  // register load card function to global
  auto function = piper::Value(
      *runtime, mock_js_app_
                    ->get(runtime.get(),
                          piper::PropNameID::forAscii(*runtime, "loadCard"))
                    .getObject(*runtime));
  runtime->global().setProperty(*runtime, "loadCard", std::move(function));

  // set init data & cache data
  lepus::Value data1 = lepus::Value::CreateObject();
  data1.SetProperty("1", lepus::Value(1));
  std::string processor_name_1 = "processor_name_1";

  lepus::Value data2 = lepus::Value::CreateObject();
  data2.SetProperty("2", lepus::Value(2));
  std::string processor_name_2 = "processor_name_2";

  lepus::Value data3 = lepus::Value::CreateObject();
  data3.SetProperty("3", lepus::Value(3));
  std::string processor_name_3 = "processor_name_3";

  std::vector<tasm::TemplateData> cache_data;
  cache_data.emplace_back(tasm::TemplateData(data2, false, processor_name_2));
  cache_data.emplace_back(tasm::TemplateData(data3, false, processor_name_3));

  std::vector<tasm::TemplateData> cache_data_copy;
  for (const auto& data : cache_data) {
    cache_data_copy.emplace_back(tasm::TemplateData::ShallowCopy(data));
  }

  // call loadApp
  app->loadApp(
      tasm::TasmRuntimeBundle{
          "",
          "",
          false,
          lepus::Value(),
          tasm::TemplateData(data1, false, processor_name_1),
          std::move(cache_data_copy),
          {},
          false,
          false,
          false,
          false,
          lepus::Value(),
      },
      lepus::Value(), tasm::PackageInstanceDSL::TT,
      tasm::PackageInstanceBundleModuleMode::EVAL_REQUIRE_MODE, "url");

  JSValueCircularArray pre_obj_{};
  // check equal
  auto lepus_args =
      *ParseJSValue(*runtime, std::move(mock_js_app_->args_ary[0][1]), nullptr,
                    "", "", pre_obj_);

  EXPECT_EQ(data1, lepus_args.GetProperty("updateData"));
  EXPECT_EQ(lepus::Value(processor_name_1.c_str()),
            lepus_args.GetProperty(tasm::kProcessorName));

  auto js_cache_data = lepus_args.GetProperty(tasm::kCacheData);
  EXPECT_EQ(data2, js_cache_data.GetProperty(0).GetProperty(tasm::kData));
  EXPECT_EQ(lepus::Value(processor_name_2),
            js_cache_data.GetProperty(0).GetProperty(tasm::kProcessorName));

  EXPECT_EQ(data3, js_cache_data.GetProperty(1).GetProperty(tasm::kData));
  EXPECT_EQ(lepus::Value(processor_name_3),
            js_cache_data.GetProperty(1).GetProperty(tasm::kProcessorName));
}

TEST_P(AppTest, OnAppReloadTest) {
  EXPECT_TRUE(app);

  // prepare TemplateData
  lepus::Value data1 = lepus::Value::CreateObject();
  data1.SetProperty("1", lepus::Value(1));
  std::string processor_name_1 = "processor_name_1";
  tasm::TemplateData template_data_1(data1, false, processor_name_1);

  // call onAppReload
  app->onAppReload(std::move(template_data_1));

  // check equal
  JSValueCircularArray pre_obj_{};
  auto lepus_args_0 =
      *ParseJSValue(*runtime, std::move(mock_js_app_->args_ary[0][0]), nullptr,
                    "", "", pre_obj_);
  auto lepus_args_1 =
      *ParseJSValue(*runtime, std::move(mock_js_app_->args_ary[0][1]), nullptr,
                    "", "", pre_obj_);

  EXPECT_EQ(data1, lepus_args_0);
  EXPECT_EQ(lepus::Value(processor_name_1.c_str()),
            lepus_args_1.GetProperty(tasm::kProcessorName));
}

TEST_P(AppTest, NotifyUpdatePageData) {
  EXPECT_TRUE(app);

  // Generate OP
  std::vector<shell::CacheDataOp> op_vec;

  lepus::Value data1 = lepus::Value::CreateObject();
  data1.SetProperty("1", lepus::Value(1));
  std::string processor_name_1 = "processor_name_1";
  shell::CacheDataType type1 = shell::CacheDataType::UPDATE;
  op_vec.emplace_back(shell::CacheDataOp(
      tasm::TemplateData(data1, false, processor_name_1), type1));

  lepus::Value data2 = lepus::Value::CreateObject();
  data2.SetProperty("2", lepus::Value(2));
  std::string processor_name_2 = "processor_name_2";
  shell::CacheDataType type2 = shell::CacheDataType::UPDATE;
  op_vec.emplace_back(shell::CacheDataOp(
      tasm::TemplateData(data2, false, processor_name_2), type2));

  lepus::Value data3 = lepus::Value::CreateObject();
  data3.SetProperty("3", lepus::Value(3));
  std::string processor_name_3 = "processor_name_3";
  shell::CacheDataType type3 = shell::CacheDataType::UPDATE;
  op_vec.emplace_back(shell::CacheDataOp(
      tasm::TemplateData(data3, false, processor_name_3), type3));

  delegate_.SetCacheDataOP(std::move(op_vec));

  // call NotifyUpdatePageData
  app->NotifyUpdatePageData();

  // check equal
  EXPECT_EQ(mock_js_app_->call_count, 3);

  JSValueCircularArray pre_obj_{};
  auto lepus_args_0_0 =
      *ParseJSValue(*runtime, std::move(mock_js_app_->args_ary[0][0]), nullptr,
                    "", "", pre_obj_);
  auto lepus_args_0_1 =
      *ParseJSValue(*runtime, std::move(mock_js_app_->args_ary[0][1]), nullptr,
                    "", "", pre_obj_);

  EXPECT_EQ(data1, lepus_args_0_0);
  EXPECT_EQ(lepus::Value(processor_name_1.c_str()),
            lepus_args_0_1.GetProperty(tasm::kProcessorName));

  auto lepus_args_1_0 =
      *ParseJSValue(*runtime, std::move(mock_js_app_->args_ary[1][0]), nullptr,
                    "", "", pre_obj_);
  auto lepus_args_1_1 =
      *ParseJSValue(*runtime, std::move(mock_js_app_->args_ary[1][1]), nullptr,
                    "", "", pre_obj_);

  EXPECT_EQ(data2, lepus_args_1_0);
  EXPECT_EQ(lepus::Value(processor_name_2.c_str()),
            lepus_args_1_1.GetProperty(tasm::kProcessorName));

  auto lepus_args_2_0 =
      *ParseJSValue(*runtime, std::move(mock_js_app_->args_ary[2][0]), nullptr,
                    "", "", pre_obj_);
  auto lepus_args_2_1 =
      *ParseJSValue(*runtime, std::move(mock_js_app_->args_ary[2][1]), nullptr,
                    "", "", pre_obj_);

  EXPECT_EQ(data3, lepus_args_2_0);
  EXPECT_EQ(lepus::Value(processor_name_3.c_str()),
            lepus_args_2_1.GetProperty(tasm::kProcessorName));
}

TEST_P(AppTest, GetEnvTest) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));
  auto js_function = function(R"--(
function getEnv(nativeApp, key) {
  return nativeApp.getEnv(key);
}
)--");

  auto result = js_function.call(
      rt,
      {native_app, static_cast<double>(
                       tasm::LynxEnv::Key::TRAIL_REMOVE_COMPONENT_EXTRA_DATA)});
  // currently, no experimental values ​​can be obtained in unit test,
  // that is, undefined will always be returned
  EXPECT_TRUE(result->isUndefined());
}

TEST_P(AppTest, GetEnvArgsCheckTest) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function getEnv(nativeApp) {
  return nativeApp.getEnv();
}
)--");

  EXPECT_CALL(*exception_handler_,
              onJSIException(HasMessage("getEnv args count must be 1")))
      .Times(1);

  std::optional<Value> result = js_function.call(rt, {native_app});

  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->isUndefined());
}

TEST_P(AppTest, GetEnvFailedTest) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function getEnv(nativeApp, key) {
  return nativeApp.getEnv(key);
}
)--");

  std::underlying_type_t<tasm::LynxEnv::Key> non_exist_key =
      base::to_underlying(tasm::LynxEnv::Key::END_MARK);

  EXPECT_CALL(*exception_handler_,
              onJSIException(HasMessage("unknown env key " +
                                        std::to_string(non_exist_key))))
      .Times(1);

  std::optional<Value> result =
      js_function.call(rt, {native_app, static_cast<double>(non_exist_key)});

  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->isUndefined());
}

TEST_P(AppTest, JSObjectDestructionObserver) {
  rt.global().setProperty(rt, "JSObjectDestructionObserverTestResult", 1);

  Function func = function(R"--(
    function setJSObjectDestructionObserverValue() {
      JSObjectDestructionObserverTestResult = 3;
    }
  )--");

  {
    JSObjectDestructionObserver observer(app,
                                         app->CreateCallBack(std::move(func)));
  }

  EXPECT_EQ(rt.global()
                .getProperty(rt, "JSObjectDestructionObserverTestResult")
                ->getNumber(),
            3);
}

TEST_P(AppTest, ReadScriptTest) {
  auto app_proxy = std::make_shared<AppProxy>(runtime, app);
  auto read_script = function(R"--(
function readScript(nativeApp, url, params) {
  return nativeApp.readScript(url, params);
}
)--");

  // readScript with `http(s)://`
  EXPECT_CALL(delegate_,
              GetJSContentFromExternal(
                  ::testing::_, ::testing::StartsWith("http"), ::testing::_))
      .Times(::testing::AtLeast(1))
      .WillRepeatedly(::testing::Return(piper::JsContent(
          "function () { return 'http' }", piper::JsContent::Type::SOURCE)));
  // Should keep http://
  auto result = read_script.call(
      rt, {Object::createFromHostObject(rt, app_proxy),
           String::createFromAscii(rt, "http://example.com/foo.js")});
  EXPECT_TRUE(result->isString());
  EXPECT_EQ(result->getString(rt).utf8(rt), "function () { return 'http' }");
  // Should keep https://
  result = read_script.call(
      rt, {Object::createFromHostObject(rt, app_proxy),
           String::createFromAscii(rt, "https://example.com/foo.js")});
  EXPECT_TRUE(result->isString());
  EXPECT_EQ(result->getString(rt).utf8(rt), "function () { return 'http' }");

  EXPECT_CALL(delegate_,
              GetJSContentFromExternal(::testing::_, ::testing::StartsWith("/"),
                                       ::testing::_))
      .Times(::testing::AtLeast(1))
      .WillRepeatedly(::testing::Return(
          piper::JsContent("function () { return 'absolute' }",
                           piper::JsContent::Type::SOURCE)));
  // Should keep /foo.js
  result = read_script.call(rt, {Object::createFromHostObject(rt, app_proxy),
                                 String::createFromAscii(rt, "/foo.js")});
  EXPECT_TRUE(result->isString());
  EXPECT_EQ(result->getString(rt).utf8(rt),
            "function () { return 'absolute' }");
  // Should add prefix '/' to bar.js
  result = read_script.call(rt, {Object::createFromHostObject(rt, app_proxy),
                                 String::createFromAscii(rt, "bar.js")});
  EXPECT_TRUE(result->isString());
  EXPECT_EQ(result->getString(rt).utf8(rt),
            "function () { return 'absolute' }");

  EXPECT_CALL(
      delegate_,
      GetJSContentFromExternal(
          ::testing::_, ::testing::StartsWith("lynx_assets://"), ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(
          piper::JsContent("function () { return 'lynx_assets' }",
                           piper::JsContent::Type::SOURCE)));
  result = read_script.call(
      rt, {Object::createFromHostObject(rt, app_proxy),
           String::createFromAscii(rt, "lynx_assets://foo.js")});
  EXPECT_TRUE(result->isString());
  EXPECT_EQ(result->getString(rt).utf8(rt),
            "function () { return 'lynx_assets' }");

  EXPECT_CALL(delegate_,
              GetJSContentFromExternal(
                  ::testing::_, ::testing::EndsWith(".json"), ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(piper::JsContent(
          R"-({"type": "json"})-", piper::JsContent::Type::SOURCE)));
  result = read_script.call(rt, {Object::createFromHostObject(rt, app_proxy),
                                 String::createFromAscii(rt, "manifest.json")});
  EXPECT_TRUE(result->isString());
  EXPECT_EQ(result->getString(rt).utf8(rt), R"-({"type": "json"})-");

  EXPECT_CALL(delegate_, GetJSContentFromExternal(::testing::Eq("CustomEntry"),
                                                  ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(
          piper::JsContent("function () { return 'CustomEntry' }",
                           piper::JsContent::Type::SOURCE)));
  result = read_script.call(
      rt, {Object::createFromHostObject(rt, app_proxy),
           String::createFromAscii(rt, "baz.js"),
           eval("(() => ({dynamicComponentEntry: 'CustomEntry'}))()").value()});

  // The following tests should not call `GetJSContent` to get real content
  EXPECT_CALL(delegate_, GetJSContentFromExternal).Times(0);

  // TODO: remove this redundant error
  EXPECT_CALL(*exception_handler_,
              onJSIException(HasMessage("Value is null, expected a String")))
      .Times(1);
  EXPECT_CALL(
      *exception_handler_,
      onJSIException(HasMessage("readScript args[0] must be a string.")))
      .Times(1);
  read_script.call(
      rt, {Object::createFromHostObject(rt, app_proxy), piper::Value::null()});

  EXPECT_CALL(*exception_handler_,
              onJSIException(HasMessage("readScript arg count must > 0")))
      .Times(1);
  auto read_script_without_args = function(R"--(
function readScriptWithoutArgs(nativeApp) {
  return nativeApp.readScript();
}
)--");
  read_script_without_args.call(rt,
                                {Object::createFromHostObject(rt, app_proxy)});

  // readScript with `http://example.com/bar.js` but file is not exits.
  EXPECT_CALL(delegate_,
              GetJSContentFromExternal(
                  ::testing::_, ::testing::StartsWith("http"), ::testing::_))
      .Times(::testing::AtLeast(1))
      .WillRepeatedly(::testing::Return(piper::JsContent(
          "file is not exist.", piper::JsContent::Type::ERROR)));
  EXPECT_CALL(
      *exception_handler_,
      onJSIException(HasMessage(
          "readScript http://example.com/bar.js error:file is not exist.")))
      .Times(1);
  result = read_script.call(
      rt, {Object::createFromHostObject(rt, app_proxy),
           String::createFromAscii(rt, "http://example.com/bar.js")});
  EXPECT_TRUE(result->isUndefined());

  // readScript with `http://example.com/bar.js` with timeout options.
  EXPECT_CALL(delegate_, GetJSContentFromExternal(
                             ::testing::_, ::testing::StartsWith("http"), 1))
      .Times(::testing::AtLeast(1))
      .WillRepeatedly(::testing::Return(
          piper::JsContent("timeout", piper::JsContent::Type::ERROR)));
  EXPECT_CALL(*exception_handler_,
              onJSIException(HasMessage(
                  "readScript http://example.com/bar.js error:timeout")))
      .Times(1);
  result = read_script.call(
      rt,
      {Object::createFromHostObject(rt, app_proxy),
       String::createFromAscii(rt, "http://example.com/bar.js"),
       eval("(() => ({dynamicComponentEntry: 'CustomEntry', timeout: 1}))()")
           .value()});
  EXPECT_TRUE(result->isUndefined());
}

TEST_P(AppTest, ReportExceptionTest) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));
  auto js_function = function(R"--(
function reportException(nativeApp, message, stack, errorCode, errorLevel) {
  return nativeApp.reportException(message, stack, errorCode, errorLevel);
}
)--");

  auto result = js_function.call(
      rt, {native_app, String::createFromAscii(rt, "foo message"),
           String::createFromAscii(rt, "bar stack"), 0,
           static_cast<double>(base::LynxErrorLevel::Error)});

  EXPECT_TRUE(result->isUndefined());
}

TEST_P(AppTest, ReportExceptionArgsCheckTest) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function reportException(nativeApp) {
  return nativeApp.reportException();
}
)--");

  EXPECT_CALL(*exception_handler_,
              onJSIException(HasMessage(
                  "the arg count of reportException must be equal to 2")))
      .Times(1);

  std::optional<Value> result = js_function.call(rt, {native_app});

  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->isUndefined());
}

TEST_P(AppTest, ReportExceptionFailedTest) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function reportException(nativeApp, message, stack, errorCode, errorLevel) {
  return nativeApp.reportException(message, stack, errorCode, errorLevel);
}
)--");

  auto result = js_function.call(
      rt, {native_app, String::createFromAscii(rt, "foo message"),
           String::createFromAscii(rt, "bar stack"), 0, -9999});

  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->isUndefined());
}

TEST_P(AppTest, GetCustomSectionSyncTest) {
  EXPECT_TRUE(app);

  // add custom sections : { "s": "Hello world"}
  tasm::TasmRuntimeBundle card_bundle;
  constexpr char kExceptRes[] = "Hello world";
  card_bundle.custom_sections = lepus::Value{lepus::Dictionary::Create({
      {base::String("s"), lepus::Value{kExceptRes}},
  })};
  app->loadApp(std::move(card_bundle), lepus::Value(),
               tasm::PackageInstanceDSL::TT,
               tasm::PackageInstanceBundleModuleMode::EVAL_REQUIRE_MODE, "url");

  // `lynx.getCustomSectionSync` func
  auto lynx_proxy = std::make_shared<piper::LynxProxy>(runtime, app);
  auto get_custom_section_sync = [this, &rt = this->rt,
                                  &lynx_proxy](const std::string& param) {
    Object obj = Object::createFromHostObject(rt, lynx_proxy);
    std::string get_custom_section_sync_call =
        "function(lynx) { return lynx.getCustomSectionSync" + param + "; }";
    return function(get_custom_section_sync_call).call(rt, obj);
  };

  // lynx.getCustomSectionSync("s") -> "Hello world"
  {
    auto res = get_custom_section_sync(R"(("s"))");
    EXPECT_TRUE(res->isString());
    auto res_str = res->asString(rt);
    ASSERT_TRUE(res_str);
    EXPECT_EQ(res_str->utf8(rt), kExceptRes);
  }

  // lynx.getCustomSectionSync("n_s") -> null
  {
    auto res = get_custom_section_sync(R"(("n_s"))");
    EXPECT_TRUE(res->isNull());
  }

  // lynx.getCustomSectionSync() -> undefined
  {
    auto res = get_custom_section_sync(R"(())");
    EXPECT_TRUE(res->isUndefined());
  }

  // lynx.getCustomSectionSync(0) -> undefined
  {
    auto res = get_custom_section_sync(R"((0))");
    EXPECT_TRUE(res->isUndefined());
  }
}

TEST(AppTest, GenerateDynamicComponentSourceUrl) {
  auto source_url = App::GenerateDynamicComponentSourceUrl(
      "https://test.js", runtime::kAppServiceJSName);
  EXPECT_EQ(source_url, "dynamic-component/https://test.js//app-service.js");
}

TEST_P(AppTest, RequestAnimationFrame) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function requestAnimationFrameError(nativeApp, arg) {
  if (arg) {
   if (arg === 1) {
     return nativeApp.requestAnimationFrame(1);
   } else if (arg === 2) {
     return nativeApp.requestAnimationFrame({});
   } else if (arg === 3) {
     function foo() {
       var foo1 = 1;
     }
     return nativeApp.requestAnimationFrame(foo);
   } else {
     throw new Error("unexpected");
   }
  } else {
    return nativeApp.requestAnimationFrame();
  }
}
)--");

  EXPECT_CALL(
      *exception_handler_,
      onJSIException(HasMessage("requestAnimationFrame arg count must be 1")))
      .Times(1);
  auto result = js_function.call(rt, {native_app});
  EXPECT_TRUE(result->isUndefined());

  native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));
  EXPECT_CALL(*exception_handler_,
              onJSIException(HasMessage("Args[0] must be a function.")))
      .Times(0);
  result = js_function.call(rt, {native_app, 1});
  EXPECT_TRUE(result->isUndefined());

  native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));
  EXPECT_CALL(*exception_handler_,
              onJSIException(HasMessage("Args[0] must be a function.")))
      .Times(1);
  result = js_function.call(rt, {native_app, 2});
  EXPECT_TRUE(result->isUndefined());

  native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));
  result = js_function.call(rt, {native_app, 3});
  EXPECT_TRUE(result->isNumber());
}

TEST_P(AppTest, CancelAnimationFrame) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function cancelAnimationFrameError(nativeApp, arg) {
  if (arg) {
     return nativeApp.cancelAnimationFrame(1);
  } else {
    return nativeApp.cancelAnimationFrame();
  }
}
)--");

  EXPECT_CALL(
      *exception_handler_,
      onJSIException(HasMessage("cancelAnimationFrame arg count must be 1")))
      .Times(1);
  auto result = js_function.call(rt, {native_app});
  EXPECT_TRUE(result->isUndefined());

  native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));
  result = js_function.call(rt, {native_app, 1});
  EXPECT_TRUE(result->isUndefined());
}

TEST_P(AppTest, AddReporterCustomInfo) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function reportException(nativeApp) {
  nativeApp.__addReporterCustomInfo({"foo": "bar"});
  return nativeApp.reportException(new Error("foo"), {"errorCode": 200, "errorLevel": 1});
}
)--");

  EXPECT_CALL(delegate_,
              OnErrorOccurred(::testing::Field(
                  &base::LynxError::custom_info_,
                  ::testing::Contains(::testing::Pair("foo", "bar")))))
      .Times(1);
  auto result = js_function.call(rt, {native_app});
  EXPECT_TRUE(result->isUndefined());
}

TEST_P(AppTest, ModuleCheck) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function checkModule(nativeApp) {
  return nativeApp.nativeModuleProxy !== undefined && nativeApp.nativeModuleProxy.ut_module !== undefined;
}
)--");

  auto result = js_function.call(rt, {native_app});
  EXPECT_TRUE(result->isBool());
  EXPECT_TRUE(result->getBool());
}

TEST_P(AppTest, ModuleLevelCheck) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function checkModule(nativeApp) {
  return nativeApp.nativeModuleProxy.ut_module.sameName();
}
)--");

  auto result = js_function.call(rt, {native_app});
  EXPECT_TRUE(result->isString());
  EXPECT_EQ(result->getString(rt).utf8(rt), "native_module");
}

TEST_P(AppTest, ModuleGetBool) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function getString(nativeApp) {
  return nativeApp.nativeModuleProxy.ut_module.getBool(true);
}
)--");

  auto result = js_function.call(rt, {native_app});
  EXPECT_TRUE(result->isBool());
  EXPECT_EQ(result->getBool(), true);
}

TEST_P(AppTest, ModuleGetString) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function getString(nativeApp) {
  return nativeApp.nativeModuleProxy.ut_module.getString("foo");
}
)--");

  auto result = js_function.call(rt, {native_app});
  EXPECT_TRUE(result->isString());
  EXPECT_EQ(result->getString(rt).utf8(rt), "foo");
}

TEST_P(AppTest, ModuleGetNumber) {
  auto js_function = function(R"--(
function getNumber(nativeApp, num) {
  return nativeApp.nativeModuleProxy.ut_module.getNumber(num);
}
)--");

  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));
  auto result = js_function.call(rt, {native_app, 1});
  EXPECT_TRUE(result->isNumber());
  EXPECT_EQ(static_cast<int>(result->getNumber()), 1);

  native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));
  result = js_function.call(rt, {native_app, -1});
  EXPECT_TRUE(result->isNumber());
  EXPECT_EQ(static_cast<int>(result->getNumber()), -1);

  native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));
  result = js_function.call(rt, {native_app, 0});
  EXPECT_TRUE(result->isNumber());
  EXPECT_EQ(static_cast<int>(result->getNumber()), 0);

  native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));
  result = js_function.call(rt, {native_app, 3.1415926});
  EXPECT_TRUE(result->isNumber());
  EXPECT_EQ(result->getNumber(), 3.1415926);
}

TEST_P(AppTest, ModuleGetArray) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function getArray(nativeApp) {
  return nativeApp.nativeModuleProxy.ut_module.getArray([1, false, "foo", null, undefined, {"bar": 1}]);
}
)--");

  auto result = js_function.call(rt, {native_app});
  EXPECT_TRUE(result->isObject());
  auto obj = result->getObject(rt);
  EXPECT_TRUE(obj.isArray(rt));
  auto arr = obj.getArray(rt);
  EXPECT_EQ(arr.size(rt), 6);

  auto arg = arr.getValueAtIndex(rt, 0);
  EXPECT_TRUE(arg->isNumber());
  EXPECT_EQ(arg->getNumber(), 1);

  arg = arr.getValueAtIndex(rt, 1);
  EXPECT_TRUE(arg->isBool());
  EXPECT_EQ(arg->getBool(), false);

  arg = arr.getValueAtIndex(rt, 2);
  EXPECT_TRUE(arg->isString());
  EXPECT_EQ(arg->getString(rt).utf8(rt), "foo");

  arg = arr.getValueAtIndex(rt, 3);
  EXPECT_TRUE(arg->isNull());

  arg = arr.getValueAtIndex(rt, 4);
  EXPECT_TRUE(arg->isNull());

  arg = arr.getValueAtIndex(rt, 5);
  EXPECT_TRUE(arg->isObject());
  auto bar_obj = arg->getObject(rt);
  arg = bar_obj.getProperty(rt, "bar");
  EXPECT_TRUE(arg->isNumber());
  EXPECT_EQ(arg->getNumber(), 1);
}

TEST_P(AppTest, ModuleGetObject) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function getObject(nativeApp) {
  return nativeApp.nativeModuleProxy.ut_module.getObject({"foo": 1, "bar": true, "str": "foo", "arr": [1], "obj": {"foo": 2}});
}
)--");

  auto result = js_function.call(rt, {native_app});
  EXPECT_TRUE(result->isObject());
  auto obj = result->getObject(rt);

  {
    auto arg = obj.getProperty(rt, "foo");
    EXPECT_TRUE(arg->isNumber());
    EXPECT_EQ(arg->getNumber(), 1);
  }

  {
    auto arg = obj.getProperty(rt, "bar");
    EXPECT_TRUE(arg->isBool());
    EXPECT_EQ(arg->getBool(), true);
  }

  {
    auto arg = obj.getProperty(rt, "str");
    EXPECT_TRUE(arg->isString());
    EXPECT_EQ(arg->getString(rt).utf8(rt), "foo");
  }

  {
    auto arg = obj.getProperty(rt, "arr");
    EXPECT_TRUE(arg->isObject());
    auto obj_arr = arg->getObject(rt);
    EXPECT_TRUE(obj_arr.isArray(rt));
    auto arr = obj_arr.getArray(rt);
    EXPECT_EQ(arr.size(rt), 1);
    arg = arr.getValueAtIndex(rt, 0);
    EXPECT_TRUE(arg->isNumber());
    EXPECT_EQ(arg->getNumber(), 1);
  }
  {
    auto arg = obj.getProperty(rt, "obj");
    EXPECT_TRUE(arg->isObject());
    auto obj_foo = arg->getObject(rt);
    arg = obj_foo.getProperty(rt, "foo");
    EXPECT_TRUE(arg->isNumber());
    EXPECT_EQ(arg->getNumber(), 2);
  }
}

TEST_P(AppTest, ModuleSyncCallback) {
  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));

  auto js_function = function(R"--(
function syncCb(nativeApp) {
  var syncCallback = false;
  nativeApp.nativeModuleProxy.ut_module.syncCallback(()=>{
    syncCallback = true;
  });
  return syncCallback;
}
)--");

  auto result = js_function.call(rt, {native_app});
  EXPECT_TRUE(result->isBool());
  EXPECT_EQ(result->getBool(), true);
}

TEST_P(AppTest, ModuleTryError) {
  auto js_function = function(R"--(
function tryError(nativeApp, code) {
  nativeApp.nativeModuleProxy.ut_module.tryError(code);
}
)--");
  const std::string module_name = "ut_module";
  const std::string method_name = "tryError";

  auto native_app = Object::createFromHostObject(
      rt, std::make_shared<AppProxy>(runtime, app));
  EXPECT_CALL(*module_delegate_, OnErrorOccurred(::testing::_)).Times(0);
  EXPECT_CALL(*module_delegate_,
              OnMethodInvoked(::testing::Eq("ut_module"),
                              ::testing::Eq(method_name), ::testing::Eq(0)))
      .Times(1);
  auto result = js_function.call(rt, {native_app, 0});

  for (int i = 1; i < ERROR_CODE_SIZE; i++) {
    native_app = Object::createFromHostObject(
        rt, std::make_shared<AppProxy>(runtime, app));
    EXPECT_CALL(*module_delegate_,
                OnErrorOccurred(::testing::Field(&base::LynxError::error_code_,
                                                 ::testing::Eq(ERROR_CODE[i]))))
        .Times(1);
    EXPECT_CALL(
        *module_delegate_,
        OnMethodInvoked(::testing::Eq("ut_module"), ::testing::Eq(method_name),
                        ::testing::Eq(ERROR_CODE[i])))
        .Times(1);
    result = js_function.call(rt, {native_app, i});
  }
}

// TODO(liyanbo.monster): open this when pub value support this.
// TEST_P(AppTest, ModuleGetCircleObject) {
//   auto native_app = Object::createFromHostObject(
//       rt, std::make_shared<AppProxy>(runtime, app));
//
//   auto js_function = function(R"--(
// function getString(nativeApp) {
//   var foo = {};
//   var bar = {};
//   foo.bar = bar;
//   bar.foo = foo;
//   return nativeApp.nativeModuleProxy.ut_module.getObject(foo);
// }
//)--");
//   auto result = js_function.call(rt, {native_app});
//   EXPECT_TRUE(result->isUndefined());
// }

INSTANTIATE_TEST_SUITE_P(
    Runtimes, AppTest, ::testing::ValuesIn(runtimeGenerators()),
    [](const ::testing::TestParamInfo<AppTest::ParamType>& info) {
      auto rt = info.param(nullptr);
      switch (rt->type()) {
        case JSRuntimeType::v8:
          return "v8";
        case JSRuntimeType::jsc:
          return "jsc";
        case JSRuntimeType::quickjs:
          return "quickjs";
      }
    });

}  // namespace test
}  // namespace piper
}  // namespace lynx
