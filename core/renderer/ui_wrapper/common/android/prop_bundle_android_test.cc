// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"

#include <jni.h>

#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "core/base/android/java_value.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android_test.h"
#include "core/renderer/utils/android/value_converter_android.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {
namespace testing {

void PropBundleAndroidTest::SetUp() {
  prop_bundle_android_ = fml::static_ref_ptr_cast<PropBundleAndroid>(
      prop_bundle_creator_.CreatePropBundle(enable_map_buffer_));
}

void PropBundleAndroidTest::TearDown() {}

base::android::JavaValue JavaOnlyMapGetJavaValueAtIndex(
    JNIEnv* env, base::android::JavaOnlyMap& map, const std::string& key) {
  base::android::ScopedLocalJavaRef<jstring> jni_key =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, key);
  return base::android::JavaOnlyMap::JavaOnlyMapGetJavaValueAtIndex(
      env, map.jni_object(), jni_key.Get());
}

TEST_P(PropBundleAndroidTest, SetNullPropsTest) {
  JNIEnv* env = base::android::AttachCurrentThread();
  prop_bundle_android_->SetNullProps("null");
  auto map = base::android::JavaOnlyMap(env, prop_bundle_android_->GetProps());
  auto value = JavaOnlyMapGetJavaValueAtIndex(env, map, "null");
  EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Null);
  EXPECT_TRUE(prop_bundle_android_->Contains("null"));
}

TEST_P(PropBundleAndroidTest, SetUint32sAndGetTest) {
  JNIEnv* env = base::android::AttachCurrentThread();
  prop_bundle_android_->SetProps("uint32", 1u);
  auto map = base::android::JavaOnlyMap(env, prop_bundle_android_->GetProps());
  auto value = JavaOnlyMapGetJavaValueAtIndex(env, map, "uint32");
  EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Int32);
  EXPECT_EQ(value.Int32(), 1);
  EXPECT_TRUE(prop_bundle_android_->Contains("uint32"));
}

TEST_P(PropBundleAndroidTest, SetInt32sAndGetTest) {
  JNIEnv* env = base::android::AttachCurrentThread();
  prop_bundle_android_->SetProps("int32", 1);
  auto map = base::android::JavaOnlyMap(env, prop_bundle_android_->GetProps());
  auto value = JavaOnlyMapGetJavaValueAtIndex(env, map, "int32");
  EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Int32);
  EXPECT_EQ(value.Int32(), 1);
  EXPECT_TRUE(prop_bundle_android_->Contains("int32"));
}

TEST_P(PropBundleAndroidTest, SetStringAndGetTest) {
  JNIEnv* env = base::android::AttachCurrentThread();
  prop_bundle_android_->SetProps("string", "testStr");
  auto map = base::android::JavaOnlyMap(env, prop_bundle_android_->GetProps());
  auto value = JavaOnlyMapGetJavaValueAtIndex(env, map, "string");
  EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::String);
  EXPECT_EQ(value.String(), "testStr");
  EXPECT_TRUE(prop_bundle_android_->Contains("string"));
}

TEST_P(PropBundleAndroidTest, SetBooleanAndGetTest) {
  JNIEnv* env = base::android::AttachCurrentThread();
  prop_bundle_android_->SetProps("boolean", true);
  auto map = base::android::JavaOnlyMap(env, prop_bundle_android_->GetProps());
  auto value = JavaOnlyMapGetJavaValueAtIndex(env, map, "boolean");
  EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Boolean);
  EXPECT_EQ(value.Bool(), true);
  EXPECT_TRUE(prop_bundle_android_->Contains("boolean"));
}

TEST_P(PropBundleAndroidTest, SetDoubleAndGetTest) {
  JNIEnv* env = base::android::AttachCurrentThread();
  prop_bundle_android_->SetProps("double", 2.2);
  auto map = base::android::JavaOnlyMap(env, prop_bundle_android_->GetProps());
  auto value = JavaOnlyMapGetJavaValueAtIndex(env, map, "double");
  EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Double);
  EXPECT_EQ(value.Double(), 2.2);
  EXPECT_TRUE(prop_bundle_android_->Contains("double"));
}

TEST_P(PropBundleAndroidTest, SetValueAndGetTest) {
  JNIEnv* env = base::android::AttachCurrentThread();

  auto dict = lepus::Dictionary::Create();
  dict->SetValue("1", 1);
  dict->SetValue("2", 2);
  dict->SetValue("3", 3);

  prop_bundle_android_->SetProps("value",
                                 pub::ValueImplLepus(lepus::Value(dict)));
  auto map = base::android::JavaOnlyMap(env, prop_bundle_android_->GetProps());
  auto value = JavaOnlyMapGetJavaValueAtIndex(env, map, "value");
  EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Map);

  auto actual_value = android::ValueConverterAndroid::ConvertJavaOnlyMapToLepus(
      env, value.Map()->jni_object());
  EXPECT_EQ(lepus::Value(dict), actual_value);
  EXPECT_TRUE(prop_bundle_android_->Contains("value"));
}

TEST_P(PropBundleAndroidTest, DirectSetValueAndGetTest) {
  JNIEnv* env = base::android::AttachCurrentThread();

  auto dict = lepus::Dictionary::Create();
  dict->SetValue("1", 1);
  dict->SetValue("2", 2);
  dict->SetValue("3", 3);
  dict->SetValue("4", lepus::Value());
  dict->SetValue("5", lepus::Value(static_cast<int64_t>(5)));
  dict->SetValue("6", lepus::Value(static_cast<uint64_t>(6)));
  dict->SetValue("7", lepus::Value(static_cast<uint32_t>(7)));
  dict->SetValue("8", 8.0);

  auto array = lepus::CArray::Create();
  array->push_back(lepus::Value(1));
  array->push_back(lepus::Value(2));
  array->push_back(lepus::Value(3));
  dict->SetValue("9", lepus::Value(array));

  size_t data_size = 16;
  std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(data_size);
  for (size_t i = 0; i < data_size; ++i) {
    data[i] = static_cast<uint8_t>(i * 2);
  }

  auto byte_array = lepus::ByteArray::Create(std::move(data), data_size);
  dict->SetValue("10", lepus::Value(byte_array));

  dict->SetValue("11", lepus::Value(true));
  dict->SetValue("12", lepus::Value(false));

  prop_bundle_android_->SetProps(pub::ValueImplLepus(lepus::Value(dict)));
  auto map = base::android::JavaOnlyMap(env, prop_bundle_android_->GetProps());
  for (int i = 1; i <= 3; ++i) {
    auto value = JavaOnlyMapGetJavaValueAtIndex(env, map, std::to_string(i));
    EXPECT_TRUE(prop_bundle_android_->Contains(std::to_string(i).c_str()));

    if (i == 4) {
      EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Null);
    } else if (i == 5) {
      EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Int64);
      EXPECT_EQ(value.Int64(), i);
    } else if (i == 6) {
      EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Int64);
      EXPECT_EQ(value.Int64(), i);
    } else if (i == 7) {
      EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Int32);
      EXPECT_EQ(value.Int32(), i);
    } else if (i == 8) {
      EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Double);
      EXPECT_EQ(value.Double(), i);
    } else if (i == 9) {
      EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Array);
      auto actual_value =
          android::ValueConverterAndroid::ConvertJavaOnlyMapToLepus(
              env, value.Array()->jni_object());
      EXPECT_EQ(lepus::Value(array), actual_value);
    } else if (i == 10) {
      EXPECT_EQ(value.type(),
                base::android::JavaValue::JavaValueType::ByteArray);
      size_t length = static_cast<size_t>(value.Length());
      EXPECT_EQ(length, data_size);
      std::unique_ptr<uint8_t[]> copy = std::make_unique<uint8_t[]>(length);
      memcpy(copy.get(), value.ArrayBuffer(), length);
      EXPECT_EQ(lepus::Value(byte_array), lepus::Value(lepus::ByteArray::Create(
                                              std::move(copy), length)));
    } else if (i == 11) {
      EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Boolean);
      EXPECT_EQ(value.Bool(), true);
    } else if (i == 12) {
      EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Boolean);
      EXPECT_EQ(value.Bool(), false);
    } else {
      EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Int32);
      EXPECT_EQ(value.Int32(), i);
    }
  }
}

TEST_P(PropBundleAndroidTest, SetArrayAndGetTest) {
  if (enable_map_buffer_) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();

  size_t data_size = 16;
  std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(data_size);
  for (size_t i = 0; i < data_size; ++i) {
    data[i] = static_cast<uint8_t>(i * 2);
  }

  auto array = lepus::CArray::Create();
  for (size_t i = 0; i < data_size; ++i) {
    array->push_back(lepus::Value(data[i]));
  }

  prop_bundle_android_->SetPropsByID(CSSPropertyID::kPropertyIDWidth,
                                     data.get(), data_size);

  auto map = base::android::JavaOnlyMap(env, prop_bundle_android_->GetProps());
  auto value = JavaOnlyMapGetJavaValueAtIndex(env, map, "width");
  EXPECT_EQ(value.type(), base::android::JavaValue::JavaValueType::Array);

  auto actual_value =
      android::ValueConverterAndroid::ConvertJavaOnlyArrayToLepus(
          env, value.Array()->jni_object());
  EXPECT_EQ(lepus::Value(array), actual_value);
  EXPECT_TRUE(prop_bundle_android_->Contains("width"));
}

TEST_P(PropBundleAndroidTest, SetEventHandlerAndGetTest) {
  auto array = lepus::CArray::Create();
  array->push_back(lepus::Value("eventName"));
  array->push_back(lepus::Value("bindEvent"));
  array->push_back(lepus::Value(true));
  array->push_back(lepus::Value("onClick"));

  prop_bundle_android_->SetEventHandler(
      pub::ValueImplLepus(lepus::Value(array)));

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::JavaOnlyArray jarray(env,
                                      prop_bundle_android_->GetEventHandlers());

  EXPECT_EQ(base::android::JavaOnlyArray::JavaOnlyArrayGetSize(
                env, jarray.jni_object()),
            1);

  auto jvalue = base::android::JavaOnlyArray::JavaOnlyArrayGetJavaValueAtIndex(
      env, jarray.jni_object(), 0);
  EXPECT_EQ(jvalue.type(), base::android::JavaValue::JavaValueType::Map);

  auto actual_value = android::ValueConverterAndroid::ConvertJavaOnlyMapToLepus(
      env, jvalue.Map()->jni_object());
  EXPECT_EQ(actual_value.GetProperty("name"), lepus::Value("eventName"));
  EXPECT_EQ(actual_value.GetProperty("type"), lepus::Value("bindEvent"));
  EXPECT_EQ(actual_value.GetProperty("function"), lepus::Value("onClick"));
  EXPECT_EQ(actual_value.GetLength(), 3);

  auto array1 = lepus::CArray::Create();
  array1->push_back(lepus::Value("eventName1"));
  array1->push_back(lepus::Value("bindEvent1"));
  array1->push_back(lepus::Value(false));
  array1->push_back(lepus::Value("onClick1"));

  prop_bundle_android_->SetEventHandler(
      pub::ValueImplLepus(lepus::Value(array1)));

  EXPECT_EQ(base::android::JavaOnlyArray::JavaOnlyArrayGetSize(
                env, jarray.jni_object()),
            2);

  jvalue = base::android::JavaOnlyArray::JavaOnlyArrayGetJavaValueAtIndex(
      env, jarray.jni_object(), 1);
  EXPECT_EQ(jvalue.type(), base::android::JavaValue::JavaValueType::Map);

  actual_value = android::ValueConverterAndroid::ConvertJavaOnlyMapToLepus(
      env, jvalue.Map()->jni_object());
  EXPECT_EQ(actual_value.GetProperty("name"), lepus::Value("eventName1"));
  EXPECT_EQ(actual_value.GetProperty("lepusType"), lepus::Value("bindEvent1"));
  EXPECT_EQ(actual_value.GetLength(), 2);

  prop_bundle_android_->ResetEventHandler();
  EXPECT_EQ(base::android::JavaOnlyArray::JavaOnlyArrayGetSize(
                env, jarray.jni_object()),
            0);
}

INSTANTIATE_TEST_SUITE_P(PropBundleAndroidTestModule, PropBundleAndroidTest,
                         ::testing::ValuesIn(prop_bundle_test_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
