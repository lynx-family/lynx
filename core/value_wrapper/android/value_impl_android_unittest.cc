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
    ASSERT_TRUE(val->ArrayBuffer() == nullptr);
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

}  // namespace pub
}  // namespace lynx
