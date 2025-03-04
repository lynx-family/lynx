// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/jsi_unittest.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_impl_piper.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace pub {

class ValueAccessorImplTest : public ::testing::Test {
 protected:
  ValueAccessorImplTest() = default;
  ~ValueAccessorImplTest() override = default;

  void SetUp() override {}

  void TearDown() override {}
};  // ValueAccessorImplTest

TEST_F(ValueAccessorImplTest, ValueAccessorImplTotalTest) {
  auto dict = lepus::Dictionary::Create();
  auto pub_value_int32 = PubLepusValue(lepus::Value((int32_t)32));
  dict->SetValue("str_key", lepus::Value("string_value"));
  dict->SetValue("bool_key", lepus::Value(true));
  dict->SetValue("int32_key", lepus::Value((int32_t)32));
  dict->SetValue("int64_key", lepus::Value((int64_t)2147483650));
  dict->SetValue("double_key", lepus::Value(214.123));
  auto pub_dict = PubLepusValue(lepus::Value(dict));
  auto dict_str = pub_dict.GetValueForKey("str_key")->str();
  auto dict_bool = pub_dict.GetValueForKey("bool_key")->Bool();
  auto dict_int32 = pub_dict.GetValueForKey("int32_key")->Int32();
  auto dict_int64 = pub_dict.GetValueForKey("int64_key")->Int64();
  auto dict_double = pub_dict.GetValueForKey("double_key")->Double();

  auto dict_empty_str = pub_dict.GetValueAtIndex(0)->str();
  ASSERT_TRUE(dict_empty_str.empty());
  EXPECT_EQ(dict_str, "string_value");
  ASSERT_TRUE(pub_dict.IsMap());
  ASSERT_TRUE(dict_bool);
  ASSERT_TRUE(dict_int32 == 32);
  ASSERT_TRUE(dict_int64 == 2147483650);
  ASSERT_TRUE(dict_double == 214.123);
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
  ASSERT_TRUE(dict_for_int32 == 32);
  ASSERT_TRUE(dict_for_int64 == 2147483650);
  ASSERT_TRUE(dict_for_double == 214.123);

  auto dic_str_value = pub_dict.GetValueForKey(std::string("str_key"));
  ASSERT_TRUE(dic_str_value->IsString());
  ASSERT_TRUE(dic_str_value->Length() == 12);
  auto type1 = dic_str_value->Type();
  auto type2 =
      pub_dict.backend_value().GetProperty(base::String("str_key")).Type();
  ASSERT_TRUE(type1 == type2);
  EXPECT_EQ(dic_str_value->str(), "string_value");
  ASSERT_TRUE(pub_dict.Contains(std::string("double_key")));
  pub_dict.Erase(std::string("double_key"));
  ASSERT_TRUE(!pub_dict.Contains(std::string("double_key")));

  auto array = lepus::CArray::Create();
  array->push_back(lepus::Value("string_value"));
  array->push_back(lepus::Value(true));
  array->push_back(lepus::Value((uint32_t)23));
  array->push_back(lepus::Value((uint64_t)2147483651));
  array->push_back(lepus::Value(123.321));
  auto pub_array = PubLepusValue(lepus::Value(array));
  pub_array.PushValueToArray(pub_dict);
  auto array_str = pub_array.GetValueAtIndex(0)->str();
  auto array_bool = pub_array.GetValueAtIndex(1)->Bool();
  auto array_uint32 = pub_array.GetValueAtIndex(2)->UInt32();
  auto array_uint64 = pub_array.GetValueAtIndex(3)->UInt64();
  auto array_double = pub_array.GetValueAtIndex(4)->Double();
  auto array_empty_str = pub_array.GetValueForKey("empty")->str();
  ASSERT_TRUE(array_empty_str.empty());
  EXPECT_EQ(array_str, "string_value");
  ASSERT_TRUE(pub_array.IsArray());
  ASSERT_TRUE(pub_array.Length() == 6);
  ASSERT_TRUE(array_bool);
  ASSERT_TRUE(array_uint32 == 23);
  ASSERT_TRUE(array_uint64 == 2147483651);
  ASSERT_TRUE(array_double == 123.321);

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
  ASSERT_TRUE(array_for_int32 == 23);
  ASSERT_TRUE(array_for_int64 == 2147483651);
  ASSERT_TRUE(array_for_double == 123.321);

  pub_array.Erase(4);
  ASSERT_TRUE(pub_array.Length() == 5);

  auto nil_value = PubLepusValue(lepus::Value());
  auto str_value = PubLepusValue(lepus::Value(std::string("string_value")));
  auto bool_value = PubLepusValue(lepus::Value(false));
  auto int32_value = PubLepusValue(lepus::Value(12));
  uint64_t num = 2147483652;
  auto uint64_value = PubLepusValue(lepus::Value(num));
  auto double_value = PubLepusValue(lepus::Value(123.123));
  ASSERT_TRUE(nil_value.IsNil());
  ASSERT_TRUE(str_value.IsString());
  EXPECT_EQ(str_value.str(), "string_value");
  ASSERT_TRUE(bool_value.IsBool());
  ASSERT_TRUE(int32_value.IsInt32());
  ASSERT_TRUE(int32_value.Int32() == 12);
  ASSERT_TRUE(uint64_value.IsUInt64());
  ASSERT_TRUE(uint64_value.UInt64() == 2147483652);
  ASSERT_TRUE(double_value.IsDouble());
  ASSERT_TRUE(double_value.Double() == 123.123);

  ASSERT_TRUE(pub::ValueUtils::ConvertValueToLepusValue(pub_value_int32)
                  .IsEqual(lepus::Value((int32_t)32)));

  PubValueFactoryDefault factory;
  auto arr_ptr = factory.CreateArray();
  arr_ptr->PushBoolToArray(true);
  arr_ptr->PushStringToArray("test");
  arr_ptr->PushNullToArray();
  ASSERT_TRUE(arr_ptr->Length() == 3);
  arr_ptr->ForeachArray([](int64_t index, const pub::Value& value) {
    if (index == 0) {
      ASSERT_TRUE(value.IsBool());
      ASSERT_TRUE(value.Bool());
    } else if (index == 1) {
      ASSERT_TRUE(value.IsString());
    } else if (index == 2) {
      ASSERT_TRUE(value.IsNil());
    }
  });
  auto map_ptr = factory.CreateMap();
  map_ptr->PushDoubleToMap("key1", 2.333);
  map_ptr->PushInt64ToMap("key2", 14);
  map_ptr->ForeachMap([](const pub::Value& key, const pub::Value& value) {
    if (key.str() == "key1") {
      ASSERT_TRUE(value.Double() == 2.333);
    } else if (key.str() == "key2") {
      ASSERT_TRUE(value.Int64() == 14);
    }
  });
  auto bool_ptr = factory.CreateBool(false);
  ASSERT_TRUE(bool_ptr->IsBool());
  ASSERT_TRUE(!bool_ptr->Bool());
  auto num_ptr = factory.CreateNumber(3.1415);
  ASSERT_TRUE(num_ptr->IsNumber());
  ASSERT_TRUE(num_ptr->Number() == 3.1415);
  auto str_ptr = factory.CreateString("test");
  ASSERT_TRUE(str_ptr->IsString());
  ASSERT_TRUE(str_ptr->str() == "test");
  std::string s("test");
  std::unique_ptr<uint8_t[]> copy = std::make_unique<uint8_t[]>(s.size());
  memcpy(copy.get(), s.data(), s.size());
  auto byte_ptr = factory.CreateArrayBuffer(std::move(copy), s.size());
  ASSERT_TRUE(byte_ptr->IsArrayBuffer());
  ASSERT_TRUE(byte_ptr->Length() == 4);

  LOGI("finish");
}

class PiperValueTests : public piper::test::JSITestBase {};

INSTANTIATE_TEST_SUITE_P(
    Runtimes, PiperValueTests,
    ::testing::ValuesIn(piper::test::runtimeGenerators()),
    [](const ::testing::TestParamInfo<PiperValueTests::ParamType>& info) {
      auto rt = info.param(nullptr);
      switch (rt->type()) {
        case piper::JSRuntimeType::v8:
          return "v8";
        case piper::JSRuntimeType::jsc:
          return "jsc";
        case piper::JSRuntimeType::quickjs:
          return "quickjs";
      }
    });

TEST_P(PiperValueTests, ValueAccessorImplPiperTest) {
  piper::Object dict(rt);
  auto pub_value_int32 = pub::ValueImplPiper(rt, piper::Value(32));
  dict.setProperty(
      rt, "str_key",
      piper::Value(piper::String::createFromUtf8(rt, "string_value")));
  dict.setProperty(rt, "bool_key", piper::Value(true));
  dict.setProperty(rt, "int32_key", piper::Value(32));
  dict.setProperty(rt, "int64_key", piper::Value((double)2147483650));
  dict.setProperty(rt, "double_key", piper::Value(214.123));
  auto pub_dict = pub::ValueImplPiper(rt, piper::Value(dict));
  auto dict_str = pub_dict.GetValueForKey("str_key")->str();
  auto dict_bool = pub_dict.GetValueForKey("bool_key")->Bool();
  auto dict_int32 = pub_dict.GetValueForKey("int32_key")->Int32();
  auto dict_int64 = pub_dict.GetValueForKey("int64_key")->Int64();
  auto dict_double = pub_dict.GetValueForKey("double_key")->Double();

  auto dict_empty_str = pub_dict.GetValueAtIndex(0)->str();
  ASSERT_TRUE(dict_empty_str.empty());
  EXPECT_EQ(dict_str, "string_value");
  ASSERT_TRUE(pub_dict.IsMap());
  ASSERT_TRUE(dict_bool);
  ASSERT_TRUE(dict_int32 == 32);
  ASSERT_TRUE(dict_int64 == 2147483650);
  ASSERT_TRUE(dict_double == 214.123);
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
  ASSERT_TRUE(dict_for_int32 == 32);
  ASSERT_TRUE(dict_for_int64 == 2147483650);
  ASSERT_TRUE(dict_for_double == 214.123);

  auto dic_str_value = pub_dict.GetValueForKey(std::string("str_key"));
  ASSERT_TRUE(dic_str_value->IsString());
  EXPECT_EQ(dic_str_value->Length(), 12);
  auto type1 = dic_str_value->Type();
  auto type2 =
      pub_dict.backend_value().getObject(rt).getProperty(rt, "str_key")->kind();
  ASSERT_TRUE(type1 == lepus::Value_String);
  ASSERT_TRUE(type2 == piper::Value::ValueKind::StringKind);
  EXPECT_EQ(dic_str_value->str(), "string_value");
  ASSERT_TRUE(pub_dict.Contains(std::string("double_key")));

  // JSI has no delete family API
  // pub_dict.Erase(std::string("double_key"));
  // ASSERT_TRUE(!pub_dict.Contains(std::string("double_key")));

  auto array = piper::Array::createWithLength(rt, 0);
  array->setValueAtIndex(
      rt, 0, piper::Value(piper::String::createFromUtf8(rt, "string_value")));
  array->setValueAtIndex(rt, 1, piper::Value(true));
  array->setValueAtIndex(rt, 2, piper::Value(23));
  array->setValueAtIndex(rt, 3, piper::Value((double)2147483651));
  array->setValueAtIndex(rt, 4, piper::Value(123.321));
  auto pub_array = pub::ValueImplPiper(rt, piper::Value(rt, *array));
  pub_array.PushValueToArray(pub_dict);
  auto array_str = pub_array.GetValueAtIndex(0)->str();
  auto array_bool = pub_array.GetValueAtIndex(1)->Bool();
  auto array_uint32 = pub_array.GetValueAtIndex(2)->UInt32();
  auto array_uint64 = pub_array.GetValueAtIndex(3)->UInt64();
  auto array_double = pub_array.GetValueAtIndex(4)->Double();
  auto array_empty_str = pub_array.GetValueForKey("empty")->str();
  ASSERT_TRUE(array_empty_str.empty());
  EXPECT_EQ(array_str, "string_value");
  ASSERT_TRUE(pub_array.IsArray());
  ASSERT_TRUE(pub_array.Length() == 6);
  ASSERT_TRUE(array_bool);
  ASSERT_TRUE(array_uint32 == 23);
  ASSERT_TRUE(array_uint64 == 2147483651);
  ASSERT_TRUE(array_double == 123.321);

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
  ASSERT_TRUE(array_for_int32 == 23);
  ASSERT_TRUE(array_for_int64 == 2147483651);
  ASSERT_TRUE(array_for_double == 123.321);

  // JSI has no delete family API
  // pub_array.Erase(4);
  // ASSERT_TRUE(pub_array.Length() == 5);

  auto nil_value = pub::ValueImplPiper(rt, piper::Value(nullptr));
  auto und_value = pub::ValueImplPiper(rt, piper::Value());
  auto str_value = pub::ValueImplPiper(
      rt, piper::Value(piper::String::createFromUtf8(rt, "string_value")));
  auto bool_value = pub::ValueImplPiper(rt, piper::Value(false));
  auto int32_value = pub::ValueImplPiper(rt, piper::Value(12));
  uint64_t num = 2147483652;
  auto uint64_value = pub::ValueImplPiper(rt, piper::Value((double)num));
  auto double_value = pub::ValueImplPiper(rt, piper::Value(123.123));
  ASSERT_TRUE(nil_value.IsNil());
  ASSERT_TRUE(und_value.IsUndefined());
  ASSERT_TRUE(str_value.IsString());
  EXPECT_EQ(str_value.str(), "string_value");
  ASSERT_TRUE(bool_value.IsBool());
  EXPECT_EQ(bool_value.str(), "false");
  ASSERT_TRUE(!int32_value.IsInt32());
  ASSERT_TRUE(int32_value.Int32() == 12);
  ASSERT_TRUE(!uint64_value.IsUInt64());
  ASSERT_TRUE(uint64_value.UInt64() == 2147483652);
  ASSERT_TRUE(double_value.IsDouble());
  ASSERT_TRUE(double_value.Double() == 123.123);

  ASSERT_TRUE(piper::Value::strictEquals(
      rt, pub::ValueUtils::ConvertValueToPiperValue(rt, pub_value_int32),
      piper::Value(32)));
  LOGI("finish");
}

TEST_P(PiperValueTests, ValueConvertTest) {
  PubValueFactoryDefault factory;
  auto arr_ptr = factory.CreateArray();
  arr_ptr->PushBoolToArray(true);
  arr_ptr->PushStringToArray("test1");
  arr_ptr->PushNullToArray();
  arr_ptr->PushInt32ToArray(10);
  arr_ptr->PushInt64ToArray(INT64_MAX - 10);
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

  auto piper_value =
      pub::ValueUtils::ConvertValueToPiperValue(rt, *(arr_ptr.get()));
  auto pub_value_piper = pub::ValueImplPiper(rt, piper_value);
  auto lepus_value = pub::ValueUtils::ConvertValueToLepusValue(pub_value_piper);
  ASSERT_TRUE(lepus_value.IsArray());
  auto lepus_array = lepus_value.Array();
  ASSERT_TRUE(lepus_array->get(0).Bool());
  ASSERT_TRUE(lepus_array->get(1).String().str() == "test1");
  ASSERT_TRUE(lepus_array->get(2).IsNil());
  ASSERT_TRUE(lepus_array->get(3).Number() == 10);
  ASSERT_TRUE(lepus_array->get(4).Number() == INT64_MAX - 10);
  auto byte_array1 = lepus_array->get(5).ByteArray();
  std::string s1(reinterpret_cast<char*>(byte_array1->GetPtr()),
                 byte_array1->GetLength());
  ASSERT_TRUE(s1 == "array_test");
  auto lepus_map = lepus_array->get(6).Table();
  ASSERT_TRUE(lepus_map->GetValue("key1").Number() == 2.333);
  ASSERT_TRUE(lepus_map->GetValue("key2").Number() == 14);
  ASSERT_TRUE(lepus_map->GetValue("key3").Bool());
  ASSERT_TRUE(lepus_map->GetValue("key4").IsNil());
  ASSERT_TRUE(lepus_map->GetValue("key5").String().str() == "test2");
  ASSERT_TRUE(lepus_map->GetValue("key6").IsByteArray());
  auto byte_array2 = lepus_map->GetValue("key6").ByteArray();
  std::string s2(reinterpret_cast<char*>(byte_array2->GetPtr()),
                 byte_array2->GetLength());
  ASSERT_TRUE(s2 == "map_test");
}

TEST_P(PiperValueTests, BigIntTest) {
  {
    auto bigint =
        piper::BigInt::createWithString(rt, std::to_string(9007199254740999));
    auto pub_value_piper_bigint =
        pub::ValueImplPiper(rt, piper::Value(*bigint));
    ASSERT_TRUE(pub_value_piper_bigint.IsInt64());
    ASSERT_TRUE(!pub_value_piper_bigint.IsMap());
    EXPECT_EQ(pub_value_piper_bigint.Int64(), 9007199254740999);

    auto pub_value_lepus_bigint =
        pub::ValueUtils::ConvertValueToLepusValue(pub_value_piper_bigint);
    ASSERT_TRUE(pub_value_lepus_bigint.IsInt64());
    EXPECT_EQ(pub_value_lepus_bigint.Int64(), 9007199254740999);
  }

  {
    auto lepus_bigint = lepus::Value(static_cast<int64_t>(9007199254740999));
    auto pub_value_lepus_bigint = PubLepusValue(lepus_bigint);
    ASSERT_TRUE(pub_value_lepus_bigint.IsInt64());
    EXPECT_EQ(pub_value_lepus_bigint.Int64(), 9007199254740999);
    auto piper_bigint =
        pub::ValueUtils::ConvertValueToPiperValue(rt, pub_value_lepus_bigint);
    ASSERT_TRUE(piper_bigint.isObject());
    ASSERT_TRUE(pub::ValueUtils::IsBigInt(rt, piper_bigint.getObject(rt)));
    auto pub_value_piper_bigint = ValueImplPiper(rt, std::move(piper_bigint));
    ASSERT_TRUE(pub_value_piper_bigint.IsInt64());
    ASSERT_TRUE(!pub_value_piper_bigint.IsMap());
    EXPECT_EQ(pub_value_piper_bigint.Int64(), 9007199254740999);
  }
}

TEST_P(PiperValueTests, ValueUtilsTest) {
  eval(
      "testData = {\"string\":\"Hello, "
      "World!\",\"number\":42,\"object\":{\"string\":\"Hello, "
      "World!\",\"number\":42.5,\"object\":{\"key\":\"value\"},\"array\":[1,"
      "\"two\",true,null],\"boolean\":true,\"null\":null,\"undefined\": "
      "undefined},\"array\":[1,"
      "\"two\",true,null,{\"string\":\"Hello, "
      "World!\",\"number\":-42.5,\"object\":{\"key\":\"value\"},\"array\":[1,"
      "\"two\",true,null],\"boolean\":true,\"null\":null}],\"boolean\":true,"
      "\"null\":null}");
  piper::Object test_data =
      rt.global().getPropertyAsObject(rt, "testData").value();
  piper::Array names = test_data.getPropertyNames(rt).value();
  EXPECT_EQ(names.size(rt), 6);

  std::shared_ptr<PubValueFactory> factory =
      std::make_shared<PubValueFactoryDefault>();
  std::unique_ptr<pub::Value> pub_lepus_value0 =
      ValueUtils::ConvertPiperObjectToPubValue(rt, test_data, factory);
  EXPECT_EQ(pub_lepus_value0->Length(), 5);
  EXPECT_TRUE(pub_lepus_value0->IsMap());

  auto pub_object = pub_lepus_value0->GetValueForKey("object");
  EXPECT_TRUE(pub_object->IsMap());
  EXPECT_EQ(pub_object->Length(), 5);
  EXPECT_TRUE(pub_object->GetValueForKey("string")->IsString());
  EXPECT_TRUE(pub_object->GetValueForKey("number")->IsNumber());
  EXPECT_TRUE(pub_object->GetValueForKey("object")->IsMap());
  EXPECT_TRUE(pub_object->GetValueForKey("array")->IsArray());
  EXPECT_TRUE(pub_object->GetValueForKey("boolean")->IsBool());

  auto pub_array = pub_lepus_value0->GetValueForKey("array");
  EXPECT_TRUE(pub_array->IsArray());
  EXPECT_EQ(pub_array->Length(), 5);
  EXPECT_TRUE(pub_array->GetValueAtIndex(0)->IsNumber());
  EXPECT_TRUE(pub_array->GetValueAtIndex(1)->IsString());
  EXPECT_TRUE(pub_array->GetValueAtIndex(2)->IsBool());
  EXPECT_TRUE(pub_array->GetValueAtIndex(3)->IsNil());
  EXPECT_TRUE(pub_array->GetValueAtIndex(4)->IsMap());

  auto pub_piper_value = pub::ValueImplPiper(rt, piper::Value(rt, test_data));
  auto lepus_value = pub::ValueUtils::ConvertValueToLepusValue(pub_piper_value);
  EXPECT_EQ(lepus_value.GetLength(), 6);
  // There is no match because the object's null or undefined value is
  // discarded.
  EXPECT_EQ(reinterpret_cast<PubLepusValue*>(pub_lepus_value0.get())
                ->backend_value()
                .IsEqual(lepus_value),
            false);
  auto pub_lepus_value = PubLepusValue(lepus_value);
  auto piper_value =
      pub::ValueUtils::ConvertValueToPiperValue(rt, pub_lepus_value);
  piper::Array names1 = piper_value.getObject(rt).getPropertyNames(rt).value();
  EXPECT_EQ(names1.size(rt), 6);
}

}  // namespace pub

}  // namespace lynx
