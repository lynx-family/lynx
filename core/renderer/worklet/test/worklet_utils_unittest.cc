// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public

#include "core/renderer/worklet/base/worklet_utils.h"

#include <memory>

#include "core/renderer/worklet/base/worklet_utils.h"
#include "core/runtime/bindings/napi/napi_environment.h"
#include "core/runtime/bindings/napi/napi_runtime_proxy_quickjs.h"
#include "core/runtime/bindings/napi/worklet/napi_loader_ui.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

class TestNapiLoaderUI : public worklet::NapiLoaderUI {
 public:
  TestNapiLoaderUI(lepus::QuickContext* context)
      : worklet::NapiLoaderUI(context){};

  void OnAttach(Napi::Env env) override { SetNapiEnvToLEPUSContext(env); }
};

class WorkletValueConverterMethods : public ::testing::Test {
 protected:
  WorkletValueConverterMethods() = default;
  ~WorkletValueConverterMethods() override { napi_environment_->Detach(); };

  void SetUp() override {
    ctx_.Initialize();

    napi_environment_ = std::make_unique<piper::NapiEnvironment>(
        std::make_unique<TestNapiLoaderUI>(&ctx_));
    auto proxy = piper::NapiRuntimeProxyQuickjs::Create(ctx_.context());
    auto napi_proxy = std::unique_ptr<piper::NapiRuntimeProxy>(
        static_cast<piper::NapiRuntimeProxy*>(proxy.release()));
    napi_proxy_ = napi_proxy.get();
    napi_environment_->SetRuntimeProxy(std::move(napi_proxy));
    napi_environment_->Attach();
  }

  void TearDown() override{};

  lepus::QuickContext ctx_;
  std::unique_ptr<piper::NapiEnvironment> napi_environment_;
  piper::NapiRuntimeProxy* napi_proxy_;
};

TEST_F(WorkletValueConverterMethods, IsLepusEqual) {
  // BOOL
  lepus::Value value_0(false);
  lepus::Value value_1(true);
  // Number
  lepus::Value value_2((int32_t)1);
  lepus::Value value_3((uint32_t)1);
  lepus::Value value_4((int64_t)1);
  lepus::Value value_5((uint64_t)1);
  lepus::Value value_6((double)1);
  // Array
  lepus::Value ary_0(lepus::CArray::Create());
  lepus::Value ary_1(lepus::CArray::Create());
  lepus::Value ary_2(lepus::CArray::Create());

  // Object
  lepus::Value obj_0(lynx::lepus::Dictionary::Create());
  lepus::Value obj_1(lynx::lepus::Dictionary::Create());
  lepus::Value obj_2(lynx::lepus::Dictionary::Create());

  ary_0.SetProperty(0, value_0);
  ary_0.SetProperty(1, value_1);
  ary_1.SetProperty(0, value_2);
  ary_1.SetProperty(1, value_3);
  ary_1.SetProperty(2, value_4);
  ary_2.SetProperty(0, ary_0);
  ary_2.SetProperty(0, ary_1);

  obj_0.SetProperty("false", value_0);
  obj_0.SetProperty("true", value_1);
  obj_1.SetProperty("num2", value_2);
  obj_1.SetProperty("num3", value_3);
  obj_1.SetProperty("num4", value_4);
  obj_2.SetProperty("obj0", obj_0);
  obj_2.SetProperty("obj1", obj_1);

  auto napi_value_0 = worklet::ValueConverter::ConvertLepusValueToNapiValue(
      napi_proxy_->Env(), value_0);
  ASSERT_TRUE(napi_value_0.IsBoolean());
  ASSERT_FALSE(napi_value_0.ToBoolean());
  auto lepus_value_convert_0 =
      worklet::ValueConverter::ConvertNapiValueToLepusValue(napi_value_0);
  ASSERT_TRUE(lepus_value_convert_0.IsEqual(value_0));

  auto napi_value_1 = worklet::ValueConverter::ConvertLepusValueToNapiValue(
      napi_proxy_->Env(), value_1);
  ASSERT_TRUE(napi_value_1.IsBoolean());
  ASSERT_TRUE(napi_value_1.ToBoolean());
  auto lepus_value_convert_1 =
      worklet::ValueConverter::ConvertNapiValueToLepusValue(napi_value_1);
  ASSERT_TRUE(lepus_value_convert_1.IsEqual(value_1));

  const auto& f_check = [this](const lepus::Value& value) {
    auto napi_value = worklet::ValueConverter::ConvertLepusValueToNapiValue(
        napi_proxy_->Env(), value);
    auto lepus_value_convert =
        worklet::ValueConverter::ConvertNapiValueToLepusValue(napi_value);
    ASSERT_TRUE(lepus_value_convert.IsEqual(value));
  };

  f_check(value_2);
  f_check(value_3);
  f_check(value_4);

  f_check(ary_0);
  f_check(ary_1);
  f_check(ary_2);

  f_check(obj_0);
  f_check(obj_1);
  f_check(obj_2);
}

TEST_F(WorkletValueConverterMethods, IsJSValueEqual) {
  // BOOL
  auto js_0 = LEPUS_NewBool(ctx_.context(), false);
  auto js_1 = LEPUS_NewBool(ctx_.context(), true);
  auto js_2 = LEPUS_NewInt32(ctx_.context(), 1);
  auto js_3 = LEPUS_NewInt64(ctx_.context(), 1);
  auto js_4 = LEPUS_NewFloat64(ctx_.context(), 1);

  lepus::Value value_0(ctx_.context(), js_0);
  lepus::Value value_1(ctx_.context(), js_1);

  // Number
  lepus::Value value_2(ctx_.context(), js_2);
  lepus::Value value_3(ctx_.context(), js_3);
  lepus::Value value_4(ctx_.context(), js_4);

  // Array
  auto js_5 = LEPUS_NewArray(ctx_.context());
  lepus::Value ary_0(ctx_.context(), js_5);
  auto js_6 = LEPUS_NewArray(ctx_.context());
  lepus::Value ary_1(ctx_.context(), js_6);
  auto js_7 = LEPUS_NewArray(ctx_.context());
  lepus::Value ary_2(ctx_.context(), js_7);

  // Object
  auto js_8 = LEPUS_NewObject(ctx_.context());
  lepus::Value obj_0(ctx_.context(), js_8);
  auto js_9 = LEPUS_NewObject(ctx_.context());
  lepus::Value obj_1(ctx_.context(), js_9);
  auto js_10 = LEPUS_NewObject(ctx_.context());
  lepus::Value obj_2(ctx_.context(), js_10);

  ary_0.SetProperty(0, value_0);
  ary_0.SetProperty(1, value_1);
  ary_1.SetProperty(0, value_2);
  ary_1.SetProperty(1, value_3);
  ary_1.SetProperty(2, value_4);
  ary_2.SetProperty(0, ary_0);
  ary_2.SetProperty(0, ary_1);

  obj_0.SetProperty("false", value_0);
  obj_0.SetProperty("true", value_1);
  obj_1.SetProperty("num2", value_2);
  obj_1.SetProperty("num3", value_3);
  obj_1.SetProperty("num4", value_4);
  obj_2.SetProperty("obj0", obj_0);
  obj_2.SetProperty("obj1", obj_1);

  auto napi_value_0 = worklet::ValueConverter::ConvertLepusValueToNapiValue(
      napi_proxy_->Env(), value_0);
  ASSERT_TRUE(napi_value_0.IsBoolean());
  ASSERT_FALSE(napi_value_0.ToBoolean());
  auto lepus_value_convert_0 =
      worklet::ValueConverter::ConvertNapiValueToLepusValue(napi_value_0);
  ASSERT_TRUE(lepus_value_convert_0.IsEqual(value_0));

  auto napi_value_1 = worklet::ValueConverter::ConvertLepusValueToNapiValue(
      napi_proxy_->Env(), value_1);
  ASSERT_TRUE(napi_value_1.IsBoolean());
  ASSERT_TRUE(napi_value_1.ToBoolean());
  auto lepus_value_convert_1 =
      worklet::ValueConverter::ConvertNapiValueToLepusValue(napi_value_1);
  ASSERT_TRUE(lepus_value_convert_1.IsEqual(value_1));

  const auto& f_check = [this](const lepus::Value& value) {
    auto napi_value = worklet::ValueConverter::ConvertLepusValueToNapiValue(
        napi_proxy_->Env(), value);
    auto lepus_value_convert =
        worklet::ValueConverter::ConvertNapiValueToLepusValue(napi_value);
    ASSERT_TRUE(lepus_value_convert.IsEqual(value));
  };

  f_check(value_2);
  f_check(value_3);
  f_check(value_4);

  f_check(ary_0);
  f_check(ary_1);
  f_check(ary_2);

  f_check(obj_0);
  f_check(obj_1);
  f_check(obj_2);

  if (!LEPUS_IsGCMode(ctx_.context())) {
    LEPUS_FreeValue(ctx_.context(), js_0);
    LEPUS_FreeValue(ctx_.context(), js_1);
    LEPUS_FreeValue(ctx_.context(), js_2);
    LEPUS_FreeValue(ctx_.context(), js_3);
    LEPUS_FreeValue(ctx_.context(), js_4);
    LEPUS_FreeValue(ctx_.context(), js_5);
    LEPUS_FreeValue(ctx_.context(), js_6);
    LEPUS_FreeValue(ctx_.context(), js_7);
    LEPUS_FreeValue(ctx_.context(), js_8);
    LEPUS_FreeValue(ctx_.context(), js_9);
    LEPUS_FreeValue(ctx_.context(), js_10);
  }
}

TEST_F(WorkletValueConverterMethods, IsPubValueEqual) {
  lepus::Value v0(false);
  lepus::Value v1(true);
  lepus::Value v2((int32_t)1);
  lepus::Value v3((uint32_t)1);
  lepus::Value v4((int64_t)1);
  lepus::Value v5((uint64_t)1);
  lepus::Value v6((double)1);

  lepus::Value ary_0(lepus::CArray::Create());
  lepus::Value ary_1(lepus::CArray::Create());
  lepus::Value ary_2(lepus::CArray::Create());

  lepus::Value obj_0(lynx::lepus::Dictionary::Create());
  lepus::Value obj_1(lynx::lepus::Dictionary::Create());
  lepus::Value obj_2(lynx::lepus::Dictionary::Create());

  ary_0.SetProperty(0, v0);
  ary_0.SetProperty(1, v1);
  ary_1.SetProperty(0, v2);
  ary_1.SetProperty(1, v3);
  ary_1.SetProperty(2, v4);
  ary_2.SetProperty(0, ary_0);
  ary_2.SetProperty(0, ary_1);

  obj_0.SetProperty("false", v0);
  obj_0.SetProperty("true", v1);
  obj_1.SetProperty("num2", v2);
  obj_1.SetProperty("num3", v3);
  obj_1.SetProperty("num4", v4);
  obj_2.SetProperty("obj0", obj_0);
  obj_2.SetProperty("obj1", obj_1);

  // BOOL
  pub::ValueImplLepus value_0(v0);
  pub::ValueImplLepus value_1(v1);

  // Number
  pub::ValueImplLepus value_2(v2);
  pub::ValueImplLepus value_3(v3);
  pub::ValueImplLepus value_4(v4);
  pub::ValueImplLepus value_5(v5);
  pub::ValueImplLepus value_6(v6);

  // Array
  pub::ValueImplLepus array0(ary_0);
  pub::ValueImplLepus array1(ary_1);
  pub::ValueImplLepus array2(ary_2);

  // Object
  pub::ValueImplLepus map0(obj_0);
  pub::ValueImplLepus map1(obj_1);
  pub::ValueImplLepus map2(obj_2);

  auto napi_value_0 = worklet::ValueConverter::ConvertPubValueToNapiValue(
      napi_proxy_->Env(), value_0);
  ASSERT_TRUE(napi_value_0.IsBoolean());
  ASSERT_FALSE(napi_value_0.ToBoolean());

  auto napi_value_1 = worklet::ValueConverter::ConvertPubValueToNapiValue(
      napi_proxy_->Env(), value_1);
  ASSERT_TRUE(napi_value_1.IsBoolean());
  ASSERT_TRUE(napi_value_1.ToBoolean());

  const auto& f_check = [this](const pub::Value& value) {
    auto napi_value = worklet::ValueConverter::ConvertPubValueToNapiValue(
        napi_proxy_->Env(), value);

    auto lepus_value_convert =
        worklet::ValueConverter::ConvertNapiValueToLepusValue(napi_value);
    auto lepus_value = pub::ValueUtils::ConvertValueToLepusValue(value);
    ASSERT_TRUE(lepus_value_convert.IsEqual(lepus_value));
  };

  f_check(value_2);
  f_check(value_3);
  f_check(value_4);

  f_check(array0);
  f_check(array1);
  f_check(array2);

  f_check(map0);
  f_check(map1);
  f_check(map2);
}

}  // namespace base
}  // namespace lynx
