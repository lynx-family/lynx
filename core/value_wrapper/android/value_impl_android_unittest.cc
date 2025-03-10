// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/value_wrapper/android/value_impl_android.h"

#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_map.h"
#include "core/base/android/java_value.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace pub {

TEST(ValueImplAndroidUnittest, FactoryAndGetter) {
  pub::ValueImplAndroidFactory factory = pub::ValueImplAndroidFactory();
  {
    auto val = pub::ValueImplAndroid(base::android::JavaValue());
    ASSERT_TRUE(val.IsNil());
    ASSERT_TRUE(!val.IsUndefined());
  }
  {
    auto val = pub::ValueImplAndroid(
        base::android::JavaValue(static_cast<int32_t>(123)));
    ASSERT_TRUE(val.IsInt32());
    ASSERT_TRUE(!val.IsUInt32());
    ASSERT_TRUE(val.Int32() == 123);
  }
  {
    auto val = pub::ValueImplAndroid(
        base::android::JavaValue(static_cast<int64_t>(123456)));
    ASSERT_TRUE(val.IsInt64());
    ASSERT_TRUE(!val.IsUInt64());
    ASSERT_TRUE(val.Int64() == 123456);
  }
  {
    auto val = factory.CreateBool(true);
    ASSERT_TRUE(val->IsBool());
    ASSERT_TRUE(val->Bool());
  }
  {
    auto val = factory.CreateNumber(0.01);
    ASSERT_TRUE(val->IsNumber());
    ASSERT_TRUE(val->Number() == 0.01);
  }
  {
    auto val = factory.CreateString(std::string("hello world"));
    ASSERT_TRUE(val->IsString());
    ASSERT_TRUE(val->str() == "hello world");
  }
  {
    auto buf = std::make_unique<uint8_t[]>(10);
    auto val = factory.CreateArrayBuffer(std::move(buf), 10);
    ASSERT_TRUE(val->IsArrayBuffer());
    ASSERT_TRUE(val->ArrayBuffer() != nullptr);
  }
  {
    auto val = factory.CreateArray();
    ASSERT_TRUE(val->IsArray());
    ASSERT_TRUE(val->GetValueAtIndex(1)->IsNil());
    ASSERT_TRUE(!val->Erase(0));
  }
  {
    auto val = factory.CreateMap();
    ASSERT_TRUE(val->IsMap());
    ASSERT_TRUE(val->GetValueForKey("hello")->IsNil());
    ASSERT_TRUE(!val->Erase("hello"));
    ASSERT_TRUE(!val->Contains("hello"));
  }
}

TEST(ValueImplAndroidUnittest, Utils) {
  auto dict = lepus::Dictionary::Create();
  dict->SetValue("str_key", lepus::Value("string_value"));
  dict->SetValue("bool_key", lepus::Value(true));
  dict->SetValue("int32_key", lepus::Value((int32_t)32));
  dict->SetValue("int64_key", lepus::Value((int64_t)2147483650));
  dict->SetValue("double_key", lepus::Value(214.123));
  dict->SetValue("null_key", lepus::Value());

  auto sub_dict = lepus::Dictionary::Create();
  sub_dict->SetValue("str_key", lepus::Value("string_value"));
  sub_dict->SetValue("bool_key", lepus::Value(true));
  sub_dict->SetValue("int32_key", lepus::Value((int32_t)32));
  sub_dict->SetValue("int64_key", lepus::Value((int64_t)2147483650));
  sub_dict->SetValue("double_key", lepus::Value(214.123));
  sub_dict->SetValue("null_key", lepus::Value());

  auto sub_arr = lepus::CArray::Create();
  sub_arr->push_back(lepus::Value("string_value"));
  sub_arr->push_back(lepus::Value(true));
  sub_arr->push_back(lepus::Value((int32_t)32));
  sub_arr->push_back(lepus::Value((int64_t)2147483650));
  sub_arr->push_back(lepus::Value(214.123));
  sub_arr->push_back(lepus::Value());

  dict->SetValue("dict_key", lepus::Value(sub_dict));
  dict->SetValue("arr_key", lepus::Value(sub_arr));

  std::shared_ptr<PubValueFactory> factory =
      std::make_shared<PubValueFactoryDefault>();
  ValueImplLepus pub_lepus_value0 = ValueImplLepus(lepus::Value(dict));
  EXPECT_EQ(pub_lepus_value0.Length(), 8);

  auto java_value =
      pub::ValueUtilsAndroid::ConvertValueToJavaValue(pub_lepus_value0);

  ValueImplAndroid pub_java_value = ValueImplAndroid(std::move(java_value));

  lepus::Value new_lepus_value =
      ValueUtils::ConvertValueToLepusValue(pub_java_value);

  EXPECT_EQ(new_lepus_value.GetLength(), 8);
  EXPECT_EQ(dict->size(), 8);
  lepus::Value old_lepus_value = lepus::Value(dict);
  EXPECT_EQ(new_lepus_value.IsEqual(lepus::Value(dict)), true);
}

TEST(ValueImplAndroidUnittest, Uint64Uint32CornerCase) {
  {
    // Since Java value only has two integer types, int32 and int64, we convert
    // uint32 to int64 for storage to prevent range overflow.
    lepus::Value uint32_lepus_val = lepus::Value(static_cast<uint32_t>(123));
    ValueImplLepus pub_lepus_value0 =
        ValueImplLepus(lepus::Value(uint32_lepus_val));

    auto java_value =
        pub::ValueUtilsAndroid::ConvertValueToJavaValue(pub_lepus_value0);

    ValueImplAndroid pub_java_value = ValueImplAndroid(std::move(java_value));

    lepus::Value new_lepus_value =
        ValueUtils::ConvertValueToLepusValue(pub_java_value);

    EXPECT_TRUE(uint32_lepus_val.IsUInt32());
    EXPECT_TRUE(new_lepus_value.IsInt64());
    EXPECT_EQ(uint32_lepus_val.UInt32(), 123);
    EXPECT_EQ(new_lepus_value.Int64(), 123);
  }

  {
    // Since Java value cannot represent the uint64 type, the uint64 type in
    // Lepus value will be represented as int64 type when converted to Java
    // value.
    lepus::Value uint64_lepus_val = lepus::Value(static_cast<uint64_t>(123456));
    ValueImplLepus pub_lepus_value0 =
        ValueImplLepus(lepus::Value(uint64_lepus_val));

    auto java_value =
        pub::ValueUtilsAndroid::ConvertValueToJavaValue(pub_lepus_value0);

    ValueImplAndroid pub_java_value = ValueImplAndroid(std::move(java_value));

    lepus::Value new_lepus_value =
        ValueUtils::ConvertValueToLepusValue(pub_java_value);

    EXPECT_TRUE(uint64_lepus_val.IsUInt64());
    EXPECT_TRUE(new_lepus_value.IsInt64());
    EXPECT_EQ(uint64_lepus_val.UInt64(), 123456);
    EXPECT_EQ(new_lepus_value.Int64(), 123456);
  }
}

TEST(ValueImplAndroidUnittest, JavaValueMapMethod) {
  // push boolean & get boolean
  ValueImplAndroidFactory factory = pub::ValueImplAndroidFactory();
  auto value_map = factory.CreateMap();
  // push boolean & get boolean
  {
    auto result1 = value_map->PushValueToMap(
        "boolean_pub_value", ValueImplAndroid(base::android::JavaValue(true)));
    auto result2 = value_map->PushBoolToMap("boolean_value", false);
    EXPECT_TRUE(result1 && result2);
    EXPECT_TRUE(value_map->Contains("boolean_pub_value"));
    EXPECT_TRUE(value_map->GetValueForKey("boolean_pub_value")->IsBool());
    EXPECT_TRUE(value_map->GetValueForKey("boolean_pub_value")->Bool());
    EXPECT_TRUE(value_map->Contains("boolean_value"));
    EXPECT_TRUE(value_map->GetValueForKey("boolean_value")->IsBool());
    EXPECT_FALSE(value_map->GetValueForKey("boolean_value")->Bool());
  }
  // push NULL & get NULL
  {
    auto result = value_map->PushValueToMap(
        "null_value", ValueImplAndroid(base::android::JavaValue()));
    EXPECT_TRUE(result);
    EXPECT_FALSE(value_map->Contains("null_value"));
    EXPECT_TRUE(value_map->GetValueForKey("null_value")->IsNil());
  }

  // push Int32 & get Int32
  {
    auto result = value_map->PushInt32ToMap("int32_value", 123);
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_map->Contains("int32_value"));
    EXPECT_TRUE(value_map->GetValueForKey("int32_value")->IsNumber());
    EXPECT_EQ(value_map->GetValueForKey("int32_value")->Int32(), 123);
  }
  // push Int64 & get Int64
  {
    auto result = value_map->PushInt64ToMap("int64_value", 123);
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_map->Contains("int64_value"));
    EXPECT_TRUE(value_map->GetValueForKey("int64_value")->IsNumber());
    EXPECT_EQ(value_map->GetValueForKey("int64_value")->Int64(), 123);
  }
  // push Double & get Double
  {
    auto result = value_map->PushDoubleToMap("double_value", 123);
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_map->Contains("double_value"));
    EXPECT_TRUE(value_map->GetValueForKey("double_value")->IsNumber());
    EXPECT_EQ(value_map->GetValueForKey("double_value")->Double(), 123);
  }
  // push String & get String
  {
    auto result = value_map->PushStringToMap("string_value", "foo");
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_map->Contains("string_value"));
    EXPECT_TRUE(value_map->GetValueForKey("string_value")->IsString());
    EXPECT_EQ(value_map->GetValueForKey("string_value")->str(), "foo");
  }
  // push ArrayBuffer & get ArrayBuffer
  {
    std::unique_ptr<uint8_t[]> arr = std::make_unique<uint8_t[]>(3);
    arr[0] = 'f';
    arr[1] = 'o';
    arr[2] = 'o';
    auto result = value_map->PushArrayBufferToMap("array_buffer_value",
                                                  std::move(arr), 3);
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_map->Contains("array_buffer_value"));
    EXPECT_TRUE(
        value_map->GetValueForKey("array_buffer_value")->IsArrayBuffer());
    auto result_arr = value_map->GetValueForKey("array_buffer_value");
    uint8_t* pub_result = result_arr->ArrayBuffer();
    EXPECT_EQ(pub_result[0], 'f');
    EXPECT_EQ(pub_result[1], 'o');
    EXPECT_EQ(pub_result[2], 'o');
  }
  // push BigInt & get BigInt
  {
    auto result =
        value_map->PushBigIntToMap("bigint_value", "9223372036854775806");
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_map->Contains("bigint_value"));
    EXPECT_TRUE(value_map->GetValueForKey("bigint_value")->IsInt64());
    EXPECT_EQ(value_map->GetValueForKey("bigint_value")->Int64(),
              9223372036854775806);
  }
}

TEST(ValueImplAndroidUnittest, JavaValueArrayMethod) {
  // push bool & get bool
  ValueImplAndroidFactory factory = pub::ValueImplAndroidFactory();
  auto value_array = factory.CreateArray();
  // push boolean & get boolean
  {
    auto result1 = value_array->PushValueToArray(
        ValueImplAndroid(base::android::JavaValue(true)));
    auto result2 = value_array->PushBoolToArray(false);
    EXPECT_TRUE(result1 && result2);
    EXPECT_TRUE(value_array->GetValueAtIndex(0)->IsBool());
    EXPECT_TRUE(value_array->GetValueAtIndex(0)->Bool());
    EXPECT_TRUE(value_array->GetValueAtIndex(1)->IsBool());
    EXPECT_FALSE(value_array->GetValueAtIndex(1)->Bool());
  }

  // push NULL & get NULL
  {
    auto result = value_array->PushValueToArray(
        ValueImplAndroid(base::android::JavaValue()));
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_array->GetValueAtIndex(2)->IsNil());
  }

  // push Int32 & get Int32
  {
    auto result = value_array->PushInt32ToArray(123);
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_array->GetValueAtIndex(3)->IsNumber());
    EXPECT_EQ(value_array->GetValueAtIndex(3)->Int32(), 123);
  }
  // push Int64 & get Int64
  {
    auto result = value_array->PushInt64ToArray(123);
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_array->GetValueAtIndex(4)->IsNumber());
    EXPECT_EQ(value_array->GetValueAtIndex(4)->Int64(), 123);
  }
  // push Double & get Double
  {
    auto result = value_array->PushDoubleToArray(123);
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_array->GetValueAtIndex(5)->IsNumber());
    EXPECT_EQ(value_array->GetValueAtIndex(5)->Double(), 123);
  }

  // push String & get String
  {
    auto result = value_array->PushStringToArray("foo");
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_array->GetValueAtIndex(6)->IsString());
    EXPECT_EQ(value_array->GetValueAtIndex(6)->str(), "foo");
  }

  // push BigInt & get BigInt
  {
    auto result = value_array->PushBigIntToArray("9223372036854775806");
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_array->GetValueAtIndex(7)->IsInt64());
    EXPECT_EQ(value_array->GetValueAtIndex(7)->Int64(), 9223372036854775806);
  }
  // push ArrayBuffer & get ArrayBuffer
  {
    std::unique_ptr<uint8_t[]> arr = std::make_unique<uint8_t[]>(3);
    arr[0] = 'f';
    arr[1] = 'o';
    arr[2] = 'o';
    auto result = value_array->PushArrayBufferToArray(std::move(arr), 3);
    EXPECT_TRUE(result);
    EXPECT_TRUE(value_array->GetValueAtIndex(8)->IsArrayBuffer());
    auto result_arr = value_array->GetValueAtIndex(8);
    uint8_t* pub_result = result_arr->ArrayBuffer();
    EXPECT_EQ(pub_result[0], 'f');
    EXPECT_EQ(pub_result[1], 'o');
    EXPECT_EQ(pub_result[2], 'o');
  }
}

}  // namespace pub
}  // namespace lynx
