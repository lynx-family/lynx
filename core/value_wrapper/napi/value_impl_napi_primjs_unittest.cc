// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/value_wrapper/napi/value_impl_napi_primjs.h"

#include "core/runtime/bindings/napi/napi_environment.h"
#include "core/runtime/bindings/napi/napi_runtime_proxy.h"
#include "core/runtime/bindings/napi/napi_runtime_proxy_quickjs.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif  // __cplusplus

namespace lynx {
namespace pub {

class ValueImplNapiPrimJSTest : public ::testing::Test {
 protected:
  ValueImplNapiPrimJSTest()
      : rt_(LEPUS_NewRuntime()), ctx_(LEPUS_NewContext(rt_)), env_{nullptr} {
    std::unique_ptr<piper::NapiRuntimeProxy> proxy =
        piper::NapiRuntimeProxyQuickjs::Create(ctx_);
    napi_env = std::make_unique<piper::NapiEnvironment>(
        std::make_unique<piper::NapiEnvironment::Delegate>());
    napi_env->SetRuntimeProxy(std::move(proxy));
    napi_env->Attach();
    env_ = napi_env->proxy()->Env();
  }
  ~ValueImplNapiPrimJSTest() {
    napi_env->Detach();
    LEPUS_FreeContext(ctx_);
    LEPUS_FreeRuntime(rt_);
  }

  void SetUp() override {}

  void TearDown() override {}

  LEPUSRuntime* rt_;
  LEPUSContext* ctx_;
  std::unique_ptr<piper::NapiEnvironment> napi_env;
  Napi::Env env_;
};  // ValueImplNapiPrimJSTest

TEST_F(ValueImplNapiPrimJSTest, ValueAccessorTest) {
  Napi::Object dict = Napi::Object::New(env_);
  dict.Set("str_key", Napi::String::New(env_, "string_value"));
  dict.Set("bool_key", Napi::Boolean::New(env_, true));
  dict.Set("int32_key", Napi::Number::New(env_, 32));
  dict.Set("int64_key", Napi::Number::New(env_, (double)2147483650));
  dict.Set("double_key", Napi::Number::New(env_, 214.123));

  auto pub_dict = pub::ValueImplNapiPrimJS(env_, dict);
  auto pub_value_int32 =
      pub::ValueImplNapiPrimJS(env_, Napi::Number::New(env_, 32));

  auto dict_str = pub_dict.GetValueForKey("str_key")->str();
  auto dict_bool = pub_dict.GetValueForKey("bool_key")->Bool();
  auto dict_int32 = pub_dict.GetValueForKey("int32_key")->Int32();
  auto dict_int64 = pub_dict.GetValueForKey("int64_key")->Int64();
  auto dict_double = pub_dict.GetValueForKey("double_key")->Double();

  EXPECT_EQ(dict_str, "string_value");
  ASSERT_TRUE(pub_dict.IsMap());
  ASSERT_TRUE(dict_bool);
  ASSERT_EQ(dict_int32, 32);
  ASSERT_EQ(dict_int64, 2147483650);
  ASSERT_DOUBLE_EQ(dict_double, 214.123);
  ASSERT_TRUE(pub_dict.GetValueForKey("int32_key")->IsEqual(pub_value_int32));

  std::string dict_for_str;
  bool dict_for_bool;
  int32_t dict_for_int32;
  int64_t dict_for_int64;
  double dict_for_double;
  pub_dict.ForeachMap([&](const pub::Value& key, const pub::Value& value) {
    if (key.str() == std::string("str_key")) {
      dict_for_str = value.str();
    } else if (key.str() == std::string("bool_key")) {
      dict_for_bool = value.Bool();
    } else if (key.str() == std::string("int32_key")) {
      dict_for_int32 = value.Int32();
    } else if (key.str() == std::string("int64_key")) {
      dict_for_int64 = value.Int64();
    } else if (key.str() == std::string("double_key")) {
      dict_for_double = value.Double();
    }
  });
  EXPECT_EQ(dict_for_str, "string_value");
  ASSERT_TRUE(dict_for_bool);
  ASSERT_EQ(dict_for_int32, 32);
  ASSERT_EQ(dict_for_int64, 2147483650);
  ASSERT_DOUBLE_EQ(dict_for_double, 214.123);

  auto dic_str_value = pub_dict.GetValueForKey(std::string("str_key"));
  ASSERT_TRUE(dic_str_value->IsString());
  EXPECT_EQ(dic_str_value->str().size(), 12);
  auto type1 = dic_str_value->Type();
  ASSERT_EQ(type1, napi_string);
  EXPECT_EQ(dic_str_value->str(), "string_value");
  ASSERT_TRUE(pub_dict.Contains(std::string("double_key")));

  Napi::Array array = Napi::Array::New(env_);
  array.Set((uint32_t)0, Napi::String::New(env_, "string_value"));
  array.Set((uint32_t)1, Napi::Boolean::New(env_, true));
  array.Set((uint32_t)2, Napi::Number::New(env_, 23));
  array.Set((uint32_t)3, Napi::Number::New(env_, (double)2147483651));
  array.Set((uint32_t)4, Napi::Number::New(env_, 123.321));

  auto pub_array = pub::ValueImplNapiPrimJS(env_, array);

  auto array_str = pub_array.GetValueAtIndex(0)->str();
  auto array_bool = pub_array.GetValueAtIndex(1)->Bool();
  auto array_uint32 = pub_array.GetValueAtIndex(2)->UInt32();
  auto array_uint64 = pub_array.GetValueAtIndex(3)->UInt64();
  auto array_double = pub_array.GetValueAtIndex(4)->Double();
  EXPECT_EQ(array_str, "string_value");
  ASSERT_TRUE(pub_array.IsArray());
  ASSERT_EQ(pub_array.Length(), 5);
  ASSERT_TRUE(array_bool);
  ASSERT_EQ(array_uint32, 23);
  ASSERT_EQ(array_uint64, 2147483651);
  ASSERT_DOUBLE_EQ(array_double, 123.321);

  std::string array_for_str;
  bool array_for_bool;
  int32_t array_for_int32;
  int64_t array_for_int64;
  double array_for_double;
  pub_array.ForeachArray([&](int64_t index, const pub::Value& value) {
    switch (index) {
      case 0:
        array_for_str = value.str();
        break;
      case 1:
        array_for_bool = value.Bool();
        break;
      case 2:
        array_for_int32 = value.UInt32();
        break;
      case 3:
        array_for_int64 = value.UInt64();
        break;
      case 4:
        array_for_double = value.Double();
        break;
      default:
        break;
    }
  });
  EXPECT_EQ(array_for_str, "string_value");
  ASSERT_TRUE(array_for_bool);
  ASSERT_EQ(array_for_int32, 23);
  ASSERT_EQ(array_for_int64, 2147483651);
  ASSERT_DOUBLE_EQ(array_for_double, 123.321);

  auto nil_value = pub::ValueImplNapiPrimJS(env_, env_.Null());
  auto und_value = pub::ValueImplNapiPrimJS(env_, env_.Undefined());
  auto str_value =
      pub::ValueImplNapiPrimJS(env_, Napi::String::New(env_, "string_value"));
  auto bool_value =
      pub::ValueImplNapiPrimJS(env_, Napi::Boolean::New(env_, false));
  auto int32_value =
      pub::ValueImplNapiPrimJS(env_, Napi::Number::New(env_, 12));
  uint64_t num = 2147483652;
  auto uint64_value =
      pub::ValueImplNapiPrimJS(env_, Napi::Number::New(env_, (double)num));
  auto double_value =
      pub::ValueImplNapiPrimJS(env_, Napi::Number::New(env_, 123.123));

  ASSERT_TRUE(nil_value.IsNil());
  ASSERT_TRUE(und_value.IsUndefined());
  ASSERT_TRUE(str_value.IsString());
  EXPECT_EQ(str_value.str(), "string_value");
  ASSERT_TRUE(bool_value.IsBool());
  ASSERT_TRUE(int32_value.IsNumber());
  ASSERT_EQ(int32_value.Int32(), 12);
  ASSERT_TRUE(uint64_value.IsNumber());
  ASSERT_EQ(uint64_value.UInt64(), 2147483652);
  ASSERT_TRUE(double_value.IsDouble());
  ASSERT_DOUBLE_EQ(double_value.Double(), 123.123);
}

TEST_F(ValueImplNapiPrimJSTest, ValueConvertTest) {
  PubValueFactoryDefault factory;
  auto arr_ptr = factory.CreateArray();
  arr_ptr->PushBoolToArray(true);
  arr_ptr->PushStringToArray("test1");
  arr_ptr->PushNullToArray();
  arr_ptr->PushInt32ToArray(10);
  std::string arr_str("array_test");
  std::unique_ptr<uint8_t[]> arr_str_ptr =
      std::make_unique<uint8_t[]>(arr_str.size());
  memcpy(arr_str_ptr.get(), arr_str.data(), arr_str.size());
  arr_ptr->PushArrayBufferToArray(std::move(arr_str_ptr), arr_str.size());
  auto map_ptr = factory.CreateMap();
  map_ptr->PushDoubleToMap("key1", 2.333);
  map_ptr->PushInt64ToMap("key2", 14);
  map_ptr->PushBoolToMap("key3", true);
  map_ptr->PushNullToMap("key4");
  map_ptr->PushStringToMap("key5", "test2");
  std::string map_str("map_test");
  std::unique_ptr<uint8_t[]> map_str_ptr =
      std::make_unique<uint8_t[]>(map_str.size());
  memcpy(map_str_ptr.get(), map_str.data(), map_str.size());
  map_ptr->PushArrayBufferToMap("key6", std::move(map_str_ptr), map_str.size());
  arr_ptr->PushValueToArray(std::move(map_ptr));

  auto napi_val = pub::ValueUtilsNapiPrimJS::ConvertPubValueToNapiValue(
      env_, *(arr_ptr.get()));
  auto pub_value_napi = pub::ValueImplNapiPrimJS(env_, napi_val);
  auto lepus_value = pub::ValueUtils::ConvertValueToLepusValue(pub_value_napi);

  ASSERT_TRUE(lepus_value.IsArray());
  auto lepus_array = lepus_value.Array();
  ASSERT_TRUE(lepus_array->get(0).Bool());
  ASSERT_TRUE(lepus_array->get(1).String().str() == "test1");
  ASSERT_TRUE(lepus_array->get(2).IsNil());
  ASSERT_TRUE(lepus_array->get(3).Number() == 10);
  auto byte_array1 = lepus_array->get(4).ByteArray();
  std::string s1(reinterpret_cast<char*>(byte_array1->GetPtr()),
                 byte_array1->GetLength());
  ASSERT_TRUE(s1 == "array_test");
  auto lepus_map = lepus_array->get(5).Table();
  ASSERT_TRUE(lepus_map->GetValue("key1").Number() == 2.333);
  ASSERT_TRUE(lepus_map->GetValue("key2").Number() == 14);
  ASSERT_TRUE(lepus_map->GetValue("key3").Bool());
  ASSERT_TRUE(lepus_map->GetValue("key4").IsNil());
  ASSERT_TRUE(lepus_map->GetValue("key5").String().str() == "test2");
  ASSERT_TRUE(lepus_map->GetValue("key6").IsByteArray());
  auto byte_array2 = lepus_map->GetValue("key6")->ByteArray();
  std::string s2(reinterpret_cast<char*>(byte_array2->GetPtr()),
                 byte_array2->GetLength());
  ASSERT_TRUE(s2 == "map_test");
}

TEST_F(ValueImplNapiPrimJSTest, ValueUtilsTest) {
  Napi::Object test_data = Napi::Object::New(env_);
  test_data.Set("string", "Hello, World!");
  test_data.Set("number", 42);
  Napi::Object inner_object = Napi::Object::New(env_);
  inner_object.Set("key", "value");
  test_data.Set("object", inner_object);

  pub::PubValueFactoryNapiPrimJS factory(env_);
  std::unique_ptr<pub::Value> pub_value =
      std::make_unique<pub::ValueImplNapiPrimJS>(env_, test_data);

  EXPECT_EQ(pub_value->Length(), 3);
  EXPECT_TRUE(pub_value->IsMap());

  auto pub_object = pub_value->GetValueForKey("object");
  EXPECT_TRUE(pub_object->IsMap());
  EXPECT_EQ(pub_object->Length(), 1);
  EXPECT_TRUE(pub_object->GetValueForKey("key")->IsString());
}

}  // namespace pub

}  // namespace lynx
