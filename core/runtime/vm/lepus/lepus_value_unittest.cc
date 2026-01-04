// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/value/base_value.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "core/runtime/vm/lepus/function.h"
#include "core/runtime/vm/lepus/js_object.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/lepus_date.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/regexp.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace lepus {
namespace test {

namespace {
static lepus::Value TestCFunction(lepus::Context* ctx, lepus::Value*, int) {
  return lepus::Value("test");
}
}  // namespace

class LepusValueTest : public ::testing::Test {
 protected:
  LepusValueTest() : quick_ctx_(QuickContext(false)) {}
  ~LepusValueTest() = default;

  QuickContext quick_ctx_;
};

TEST_F(LepusValueTest, LepusValueNull) {
  lepus::Value v1;
  ASSERT_TRUE(v1.IsNil());
  ASSERT_TRUE(!v1.IsReference());
  ASSERT_TRUE(v1.IsFalse());
  ASSERT_TRUE(v1.Type() == Value_Nil);
  lepus::Value v4 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), LEPUS_NULL);
  ASSERT_TRUE(v4.IsNil());
  ASSERT_TRUE(v4.IsEmpty());
  ASSERT_TRUE(v4.IsJsNull());
  ASSERT_TRUE(v4.IsFalse());
  lepus::Value v5;
  v5.SetNil();
  ASSERT_TRUE(v5.IsNil());
  ASSERT_TRUE(v5.IsEmpty());

  lepus::Value v6;
  v6.SetUndefined();
  ASSERT_TRUE(v6.IsUndefined());
  ASSERT_TRUE(v6.IsEmpty());
  ASSERT_TRUE(v6.IsFalse());
  ASSERT_TRUE(v6.Type() == Value_Undefined);
  lepus::Value v7 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), LEPUS_UNDEFINED);
  ASSERT_TRUE(v7.IsUndefined());

  lepus::Value v8 = v7;
  ASSERT_TRUE(v8.IsUndefined());
  lepus::Value v9(std::move(v8));
  ASSERT_TRUE(v9.IsUndefined());
  v9 = std::move(v7);
  ASSERT_TRUE(v9.IsUndefined());
  lepus::Value v10 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), LEPUS_UNDEFINED);
  ASSERT_TRUE(v10.IsUndefined());
  ASSERT_TRUE(v10.IsEmpty());
  ASSERT_TRUE(v10.IsJSUndefined());
  ASSERT_TRUE(v10.IsFalse());
}

TEST_F(LepusValueTest, LepusValueNumber) {
  {
    lepus::Value v1(true, true);
    ASSERT_TRUE(v1.IsNaN());
    ASSERT_TRUE(v1.NaN());
    ASSERT_TRUE(v1.Type() == Value_NaN);
    lepus::Value v2;
    v2.SetNan(true);
    ASSERT_TRUE(v2.IsNaN());
    ASSERT_TRUE(v2.NaN());
    ASSERT_TRUE(v2.IsFalse());
  }
  {
    lepus::Value v3((int32_t)10);
    ASSERT_TRUE(v3.IsInt32());
    ASSERT_TRUE(v3.IsNumber());
    ASSERT_TRUE(v3.Int32() == 10);
    ASSERT_TRUE(v3.Type() == Value_Int32);
    v3.SetNumber((uint32_t)100);
    ASSERT_TRUE(v3.IsUInt32());
    ASSERT_TRUE(v3.UInt32() == 100);
    ASSERT_TRUE(v3.Type() == Value_UInt32);
    lepus::Value v4((uint32_t)50);
    ASSERT_TRUE(v4.IsUInt32());
    ASSERT_TRUE(v4.IsNumber());
    ASSERT_TRUE(v4.UInt32() == 50);
    v4.SetNumber((int32_t)101);
    ASSERT_TRUE(v4.IsInt32());
    ASSERT_TRUE(v4.Int32() == 101);
    int64_t num1 = (int64_t)INT32_MAX + 1;
    lepus::Value v5(num1);
    ASSERT_TRUE(v5.IsInt64());
    ASSERT_TRUE(v5.IsNumber());
    ASSERT_TRUE(v5.Int64() == num1);
    ASSERT_TRUE(v5.Type() == Value_Int64);
    v5.SetNumber((int32_t)101);
    ASSERT_TRUE(v5.IsInt32());
    ASSERT_TRUE(v5.Int32() == 101);
    uint64_t num2 = (uint64_t)INT32_MAX + 10;
    lepus::Value v6(num2);
    ASSERT_TRUE(v6.IsUInt64());
    ASSERT_TRUE(v6.IsNumber());
    ASSERT_TRUE(v6.UInt64() == num2);
    ASSERT_TRUE(v6.Type() == Value_UInt64);
    v6.SetNumber((int32_t)101);
    ASSERT_TRUE(v6.IsInt32());
    ASSERT_TRUE(v6.Int32() == 101);
    lepus::Value v7(3.14f);
    ASSERT_TRUE(v7.IsDouble());
    ASSERT_TRUE(v7.IsNumber());
    ASSERT_TRUE(v7.Double() == 3.14f);
    ASSERT_TRUE(v7.Type() == Value_Double);
    v7.SetNumber((int32_t)101);
    ASSERT_TRUE(v7.IsInt32());
    ASSERT_TRUE(v7.Int32() == 101);
    lepus::Value v8((uint8_t)3);
    ASSERT_TRUE(v8.IsUInt32());
    ASSERT_TRUE(v8.UInt32() == 3);
  }
  {
    int64_t num = (int64_t)INT32_MAX + 5;
    LEPUSValue val_int64 = LEPUS_NewInt64(quick_ctx_.context(), num);
    lepus::Value v9 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_int64);
    ASSERT_TRUE(v9.IsNumber());
    ASSERT_TRUE(v9.IsJSNumber());
    ASSERT_TRUE(v9.IsJSInteger());
    ASSERT_TRUE(v9.Number() == num);
    ASSERT_TRUE(v9.JSInteger() == num);
    ASSERT_TRUE(v9.Int64() == num);
    // ASSERT_TRUE(v9.Type() == Value_Nil);

    int32_t num1 = 2345;
    LEPUSValue val_int32 = LEPUS_NewInt32(quick_ctx_.context(), num1);
    lepus::Value v10 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_int32);
    ASSERT_TRUE(v10.IsNumber());
    ASSERT_TRUE(v10.IsJSNumber());
    ASSERT_TRUE(v10.IsJSInteger());
    ASSERT_TRUE(v10.Number() == num1);
    ASSERT_TRUE(v10.JSInteger() == num1);
    ASSERT_TRUE(v10.Int64() == num1);

    double num2 = 3456.0f;
    LEPUSValue val_double = LEPUS_NewFloat64(quick_ctx_.context(), num2);
    lepus::Value v11 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_double);
    ASSERT_TRUE(v11.IsNumber());
    ASSERT_TRUE(v11.IsJSNumber());
    ASSERT_TRUE(v11.IsJSInteger());
    ASSERT_TRUE(v11.Number() == num2);
    ASSERT_TRUE(v11.JSInteger() == num2);
    ASSERT_TRUE(v11.Int64() == num2);

    lepus::Value v12(0);
    ASSERT_TRUE(v12.IsFalse());
    LEPUSValue val_int32_0 = LEPUS_NewInt32(quick_ctx_.context(), 0);
    lepus::Value v13 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_int32_0);
    ASSERT_TRUE(v13.IsFalse());

    LEPUS_FreeValue(quick_ctx_.context(), val_int64);
    LEPUS_FreeValue(quick_ctx_.context(), val_int32);
    LEPUS_FreeValue(quick_ctx_.context(), val_double);
    LEPUS_FreeValue(quick_ctx_.context(), val_int32_0);
  }
  {
    lepus::Value v14((int32_t)10);
    lepus::Value v15((int32_t)5);
    ASSERT_TRUE((v14 / v15).Number() == 2);
    ASSERT_TRUE((v14 * v15).Number() == 50);
    ASSERT_TRUE((v14 + v15).Number() == 15);
    ASSERT_TRUE((v14 - v15).Number() == 5);
    ASSERT_TRUE((v14 % v15).Number() == 0);
  }
}

TEST_F(LepusValueTest, LepusValueString) {
  {
    base::String s1("test");
    lepus::Value v1(s1);
    ASSERT_TRUE(v1.IsString());
    ASSERT_TRUE(v1.StdString() == "test");
    ASSERT_TRUE(v1.GetLength() == 4);
    ASSERT_TRUE(v1.IsReference());
    ASSERT_TRUE(v1.Type() == Value_String);

    lepus::Value b1(true);
    ASSERT_TRUE(b1.IsBool());
    ASSERT_TRUE(b1.Type() == Value_Bool);
    ASSERT_TRUE(b1.StdString() == "true");

    LEPUSValue val_str1 =
        LEPUS_NewStringLen(quick_ctx_.context(), "test_val", 8);
    LEPUSRefCountHeader* p =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
    ASSERT_TRUE(p->ref_count == 1);
    lepus::Value v2 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str1);
    ASSERT_TRUE(v2.IsString());
    ASSERT_TRUE(v2.IsJSString());
    ASSERT_TRUE(v2.GetLength() == 8);
    ASSERT_TRUE(v2.StdString() == "test_val");
    ASSERT_TRUE(v2.ToString() == "test_val");
    LEPUSValue val_bool = LEPUS_NewBool(quick_ctx_.context(), false);
    lepus::Value b2 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_bool);
    ASSERT_TRUE(b2.IsBool());
    ASSERT_TRUE(b2.StdString() == "false");

    lepus::Value v3(std::move(v1));
    ASSERT_TRUE(v3.IsString());
    ASSERT_TRUE(v3.StdString() == "test");
    lepus::Value v4(std::move(v2));
    ASSERT_TRUE(v4.IsString());
    ASSERT_TRUE(v4.StdString() == "test_val");
    LEPUSRefCountHeader* p1 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
    ASSERT_TRUE(p1->ref_count == 2);

    LEPUSValue val_str2 =
        LEPUS_NewStringLen(quick_ctx_.context(), "test_val", 8);
    lepus::Value v5 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str2);
    ASSERT_TRUE(v5.IsString());
    ASSERT_TRUE(v5.StdString() == "test_val");
    LEPUSRefCountHeader* p2 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str2);
    ASSERT_TRUE(p2->ref_count == 2);

    base::String s2("test2");
    lepus::Value v6(std::move(s2));
    ASSERT_TRUE(v6.IsString());
    ASSERT_TRUE(v6.StdString() == "test2");

    LEPUS_FreeValue(quick_ctx_.context(), val_str1);
    LEPUS_FreeValue(quick_ctx_.context(), val_bool);
    LEPUS_FreeValue(quick_ctx_.context(), val_str2);
  }

  {
    lepus::Value v7("abcd");
    ASSERT_TRUE(v7.IsString());
    ASSERT_TRUE(v7.StdString() == "abcd");
    lepus::Value v8(v7);
    ASSERT_TRUE(v8.IsString());
    ASSERT_TRUE(v8.StdString() == "abcd");
    auto str = v8.String();
    ASSERT_TRUE(str.str() == "abcd");
    auto str1 = lepus::Value(v8).String();
    ASSERT_TRUE(str1.str() == "abcd");
  }

  {
    lepus::Value v9;
    base::String s3("test");
    v9.SetString(s3);
    ASSERT_TRUE(v9.IsString());
    ASSERT_TRUE(v9.StdString() == "test");
    ASSERT_TRUE(v9.ToString() == "test");

    lepus::Value v10;
    v10.SetString(base::String("aaa"));
    ASSERT_TRUE(v10.IsString());
    ASSERT_TRUE(v10.StdString() == "aaa");

    auto r2 = v9.GetProperty(2);
    ASSERT_TRUE(r2.StdString() == "s");

    lepus::Value v11("");
    ASSERT_TRUE(v11.IsFalse());
  }
}

TEST_F(LepusValueTest, LepusValueArray) {
  {
    auto arr1 = lepus::CArray::Create();
    arr1->push_back(lepus::Value((int32_t)101));
    arr1->push_back(lepus::Value((int64_t)2001));
    arr1->push_back(lepus::Value(5.645f));
    arr1->push_back(lepus::Value(false));
    arr1->push_back(lepus::Value("testing"));
    lepus::Value v1(arr1);
    ASSERT_TRUE(v1.Type() == Value_Array);
    ASSERT_TRUE(v1.IsArray());
    auto r0 = v1.GetProperty(0);
    ASSERT_TRUE(r0.Int32() == 101);
    auto r1 = v1.GetProperty(1);
    ASSERT_TRUE(r1.Int64() == 2001);
    auto r2 = v1.GetProperty(2);
    ASSERT_TRUE(r2.Double() == 5.645f);
    auto r3 = v1.GetProperty(3);
    ASSERT_TRUE(r3.Bool() == false);
    auto r4 = v1.GetProperty(4);
    ASSERT_TRUE(r4.StdString() == "testing");

    v1.SetProperty(5, lepus::Value(true));
    auto r5 = v1.GetProperty(5);
    ASSERT_TRUE(r5.Bool() == true);
    lepus::Value id6("666");
    v1.SetProperty(6, id6);
    auto r6 = v1.GetProperty(6);
    ASSERT_TRUE(r6.StdString() == "666");
    ASSERT_TRUE(v1.GetLength() == 7);
    ASSERT_TRUE(v1.Array()->size() == 7);
  }
  LEPUSValue val_str1 = LEPUS_NewStringLen(quick_ctx_.context(), "test_val", 8);
  {
    auto arr2 = lepus::CArray::Create();
    arr2->push_back(MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str1));
    arr2->push_back(lepus::Value("testing"));
    lepus::Value v2(arr2);
    LEPUSRefCountHeader* p1 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
    ASSERT_TRUE(p1->ref_count == 2);
    auto r0 = v2.GetProperty(0);
    ASSERT_TRUE(r0.StdString() == "test_val");
    LEPUSRefCountHeader* p2 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
    ASSERT_TRUE(p2->ref_count == 3);
    auto r1 = v2.GetProperty(1);
    ASSERT_TRUE(r1.StdString() == "testing");

    LEPUSValue val_str2 = LEPUS_NewStringLen(quick_ctx_.context(), "str2", 4);
    v2.SetProperty(2, MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str2));
    LEPUSRefCountHeader* p3 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str2);
    ASSERT_TRUE(p3->ref_count == 2);
    auto r2 = v2.GetProperty(2);
    ASSERT_TRUE(r2.StdString() == "str2");
    LEPUSRefCountHeader* p4 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str2);
    ASSERT_TRUE(p4->ref_count == 3);
    int64_t num = (int64_t)INT32_MAX + 3;
    LEPUSValue val_int64 = LEPUS_NewInt64(quick_ctx_.context(), num);
    lepus::Value id3 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_int64);
    v2.SetProperty(3, id3);
    auto r3 = v2.GetProperty(3);
    ASSERT_TRUE(r3.Number() == num);
    ASSERT_TRUE(v2.GetLength() == 4);

    LEPUS_FreeValue(quick_ctx_.context(), val_str2);
    LEPUS_FreeValue(quick_ctx_.context(), val_int64);
  }
  LEPUSRefCountHeader* p0 = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
  ASSERT_TRUE(p0->ref_count == 1);
  LEPUS_FreeValue(quick_ctx_.context(), val_str1);

  {
    std::vector<LEPUSValue> values;
    values.reserve(10);
    for (int32_t i = 0; i < 10; i++) {
      values.emplace_back(LEPUS_NewInt32(quick_ctx_.context(), i));
    }
    LEPUSValue val_arr =
        LEPUS_NewArrayWithValue(quick_ctx_.context(), 10, values.data());
    lepus::Value arr3 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_arr);
    arr3.IteratorJSValue(
        [](const lepus::Value& key, const lepus::Value& element) {
          ASSERT_TRUE(key.Int32() == element.Int32());
        });

    LEPUSValue val_str3 = LEPUS_NewStringLen(quick_ctx_.context(), "str3", 4);
    arr3.SetProperty(10, MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str3));
    auto r10 = arr3.GetProperty(10);
    ASSERT_TRUE(r10.StdString() == "str3");

    LEPUS_FreeValue(quick_ctx_.context(), val_arr);
  }
}

TEST_F(LepusValueTest, LepusValueMap) {
  {
    auto dict = lepus::Dictionary::Create();
    dict->SetValue("key1", lepus::Value((int32_t)101));
    dict->SetValue("key2", lepus::Value((int64_t)2001));
    dict->SetValue("key3", lepus::Value(5.645f));
    dict->SetValue("key4", lepus::Value(false));
    dict->SetValue("key5", lepus::Value("testing"));
    lepus::Value v1(dict);
    ASSERT_TRUE(v1.Type() == Value_Table);
    ASSERT_TRUE(v1.IsTable());
    auto r0 = v1.GetProperty("key1");
    ASSERT_TRUE(r0.Int32() == 101);
    auto r1 = v1.GetProperty("key2");
    ASSERT_TRUE(r1.Int64() == 2001);
    auto r2 = v1.GetProperty("key3");
    ASSERT_TRUE(r2.Double() == 5.645f);
    auto r3 = v1.GetProperty("key4");
    ASSERT_TRUE(r3.Bool() == false);
    auto r4 = v1.GetProperty("key5");
    ASSERT_TRUE(r4.StdString() == "testing");

    v1.SetProperty("key6", lepus::Value(true));
    auto r5 = v1.GetProperty("key6");
    ASSERT_TRUE(r5.Bool() == true);
    lepus::Value id6("666");
    v1.SetProperty("key7", id6);
    auto r6 = v1.GetProperty("key7");
    ASSERT_TRUE(r6.StdString() == "666");
    lepus::Value id7("abc");
    base::String key8("key8");
    v1.SetProperty(key8, id7);
    auto r7 = v1.GetProperty("key8");
    ASSERT_TRUE(r7.StdString() == "abc");
    ASSERT_TRUE(v1.GetLength() == 8);
    ASSERT_TRUE(v1.Table()->size() == 8);
    ASSERT_TRUE(v1.Contains("key1"));
  }
  LEPUSValue val_str1 = LEPUS_NewStringLen(quick_ctx_.context(), "test_val", 8);
  {
    auto dict2 = lepus::Dictionary::Create();
    dict2->SetValue("key1", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str1));
    dict2->SetValue("key2", lepus::Value("testing"));
    lepus::Value v2(dict2);
    ASSERT_TRUE(v2.Contains("key1"));
    LEPUSRefCountHeader* p1 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
    ASSERT_TRUE(p1->ref_count == 2);
    auto r0 = v2.GetProperty("key1");
    ASSERT_TRUE(r0.StdString() == "test_val");
    LEPUSRefCountHeader* p2 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
    ASSERT_TRUE(p2->ref_count == 3);
    auto r1 = v2.GetProperty("key2");
    ASSERT_TRUE(r1.StdString() == "testing");

    LEPUSValue val_str2 = LEPUS_NewStringLen(quick_ctx_.context(), "str2", 4);
    v2.SetProperty("key3", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str2));
    LEPUSRefCountHeader* p3 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str2);
    ASSERT_TRUE(p3->ref_count == 2);
    auto r2 = v2.GetProperty("key3");
    ASSERT_TRUE(r2.StdString() == "str2");
    LEPUSRefCountHeader* p4 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str2);
    ASSERT_TRUE(p4->ref_count == 3);
    int64_t num = (int64_t)INT32_MAX + 3;
    LEPUSValue val_int64 = LEPUS_NewInt64(quick_ctx_.context(), num);
    lepus::Value id3 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_int64);
    v2.SetProperty("key4", id3);
    auto r3 = v2.GetProperty("key4");
    ASSERT_TRUE(r3.Number() == num);
    ASSERT_TRUE(v2.GetLength() == 4);

    LEPUS_FreeValue(quick_ctx_.context(), val_str2);
    LEPUS_FreeValue(quick_ctx_.context(), val_int64);
  }
  LEPUSRefCountHeader* p0 = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
  ASSERT_TRUE(p0->ref_count == 1);
  LEPUS_FreeValue(quick_ctx_.context(), val_str1);

  LEPUSValue val_str3 = LEPUS_NewStringLen(quick_ctx_.context(), "str2", 4);
  {
    LEPUSValue val_map = LEPUS_NewObject(quick_ctx_.context());
    lepus::Value dict3 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_map);
    LEPUSValue val_bool = LEPUS_NewBool(quick_ctx_.context(), true);
    dict3.SetProperty("key1",
                      MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str3));
    dict3.SetProperty("key2",
                      MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_bool));
    dict3.IteratorJSValue(
        [](const lepus::Value& key, const lepus::Value& element) {
          if (key.StdString() == "key1") {
            ASSERT_TRUE(element.IsString());
          } else if (key.StdString() == "key2") {
            ASSERT_TRUE(element.IsBool());
          } else {
            ASSERT_TRUE(false);
          }
        });

    LEPUS_FreeValue(quick_ctx_.context(), val_map);
    LEPUS_FreeValue(quick_ctx_.context(), val_bool);
  }
  LEPUSRefCountHeader* p5 = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str3);
  ASSERT_TRUE(p5->ref_count == 1);
  LEPUS_FreeValue(quick_ctx_.context(), val_str3);
}

TEST_F(LepusValueTest, LepusValueArrayBuffer) {
  auto buffer1 = lepus::ByteArray::Create();
  lepus::Value v1(buffer1);
  ASSERT_TRUE(v1.IsByteArray());
  auto buffer2 = lepus::ByteArray::Create();
  lepus::Value v2(std::move(buffer2));
  ASSERT_TRUE(v2.IsByteArray());
  ASSERT_TRUE(v2.ByteArray());
  ASSERT_TRUE(v2.Type() == Value_ByteArray);
}

TEST_F(LepusValueTest, LepusValueRefCounted) {
  {
    auto obj1 = lepus::LEPUSObject::Create();
    lepus::Value v1(obj1);
    ASSERT_TRUE(v1.IsJSObject());
    auto obj2 = lepus::LEPUSObject::Create();
    lepus::Value v2(std::move(obj2));
    ASSERT_TRUE(v2.IsJSObject());

    auto obj3 = lepus::LEPUSObject::Create();
    lepus::Value v3;
    v3.SetRefCounted(obj3);
    ASSERT_TRUE(v3.IsJSObject());
    lepus::Value v4;
    v4.SetRefCounted(std::move(obj3));
    ASSERT_TRUE(v4.IsJSObject());
    ASSERT_TRUE(v4.RefCounted());
    ASSERT_TRUE(v4.Type() == Value_JSObject);
  }
  {
    auto js_object = lepus::LEPUSObject::Create();
    fml::RefPtr<lepus::RefCounted> ref_counted = js_object;
    lepus::Value v1(ref_counted);
    ASSERT_TRUE(v1.IsRefCounted());
    lepus::Value v2;
    v2.SetRefCounted(ref_counted);
    ASSERT_TRUE(v2.IsRefCounted());
    lepus::Value v3;
    v3.SetRefCounted(std::move(ref_counted));
    ASSERT_TRUE(v3.IsRefCounted());
  }
  {
    auto s1 = lepus::Closure::Create(lepus::Function::Create());
    lepus::Value v1(s1);
    ASSERT_TRUE(v1.IsClosure());
    lepus::Value v2(std::move(s1));
    ASSERT_TRUE(v2.IsClosure());
    lepus::Value v3(std::move(v2));
    ASSERT_TRUE(v3.IsClosure());
    auto s2 = lepus::Closure::Create(lepus::Function::Create());
    lepus::Value v4;
    v4.SetRefCounted(s2);
    ASSERT_TRUE(v4.IsClosure());
    lepus::Value v5;
    v5.SetRefCounted(lepus::Closure::Create(lepus::Function::Create()));
    ASSERT_TRUE(v5.IsClosure());
    ASSERT_TRUE(v5.Type() == Value_Closure);
    auto r1 = v5.RefCounted();
    ASSERT_TRUE(r1);
  }
  {
    auto s1 = lepus::CDate::Create();
    lepus::Value v1(s1);
    ASSERT_TRUE(v1.IsCDate());
    lepus::Value v2(std::move(s1));
    ASSERT_TRUE(v2.IsCDate());
    lepus::Value v3(std::move(v2));
    ASSERT_TRUE(v3.IsCDate());
    auto s2 = lepus::CDate::Create();
    lepus::Value v4;
    v4.SetRefCounted(s2);
    ASSERT_TRUE(v4.IsCDate());
    lepus::Value v5;
    v5.SetRefCounted(lepus::CDate::Create());
    ASSERT_TRUE(v5.IsCDate());
    ASSERT_TRUE(v5.Type() == Value_CDate);
    auto r1 = v5.RefCounted();
    ASSERT_TRUE(r1);
  }
  {
    auto s1 = lepus::RegExp::Create();
    lepus::Value v1(s1);
    ASSERT_TRUE(v1.IsRegExp());
    lepus::Value v2(std::move(s1));
    ASSERT_TRUE(v2.IsRegExp());
    lepus::Value v3(std::move(v2));
    ASSERT_TRUE(v3.IsRegExp());
    auto s2 = lepus::RegExp::Create();
    lepus::Value v4;
    v4.SetRefCounted(s2);
    ASSERT_TRUE(v4.IsRegExp());
    lepus::Value v5;
    v5.SetRefCounted(lepus::RegExp::Create());
    ASSERT_TRUE(v5.IsRegExp());
    ASSERT_TRUE(v5.Type() == Value_RegExp);
    auto r1 = v5.RefCounted();
    ASSERT_TRUE(r1);
  }
}

TEST_F(LepusValueTest, LepusValuePointer) {
  {
    int* a = new int;
    *a = 10;
    lepus::Value v1(a);
    ASSERT_TRUE(v1.IsCPointer());
    ASSERT_TRUE(v1.Type() == Value_CPointer);
    int* b = reinterpret_cast<int*>(v1.CPoint());
    ASSERT_TRUE(*a == *b);

    lepus::Value v2;
    int* c = new int;
    v2.SetCPoint(c);
    int* d = reinterpret_cast<int*>(v2.CPoint());
    ASSERT_TRUE(v2.IsCPointer());
    ASSERT_TRUE(!v2.IsReference());
    ASSERT_TRUE(*c == *d);
  }

  {
    lepus::Value v3(&TestCFunction);
    ASSERT_TRUE(v3.IsCFunction());
    ASSERT_TRUE(v3.Type() == Value_CFunction);
    ASSERT_TRUE(!v3.IsReference());
    auto func = v3.Function();
    auto ret = func(&quick_ctx_, nullptr, 0);
    ASSERT_TRUE(ret.IsString());

    lepus::Value v4;
    v4.SetCFunction(&TestCFunction);
    ASSERT_TRUE(v4.IsCFunction());
  }
}

TEST_F(LepusValueTest, LepusValueToBaseValue) {
  {
    LEPUSValue val_str1 =
        LEPUS_NewStringLen(quick_ctx_.context(), "test_val", 8);
    LEPUSValue val_str2 =
        LEPUS_NewStringLen(quick_ctx_.context(), "test_abc", 8);
    int64_t num = (int64_t)INT32_MAX + 3;
    LEPUSValue val_int64 = LEPUS_NewInt64(quick_ctx_.context(), num);
    LEPUSValue val_bool = LEPUS_NewBool(quick_ctx_.context(), true);
    LEPUSValue val_int = LEPUS_NewInt32(quick_ctx_.context(), 111);
    LEPUSValue val_arr = LEPUS_NewArray(quick_ctx_.context());
    LEPUSValue val_obj = LEPUS_NewObject(quick_ctx_.context());
    LEPUSValue val_double = LEPUS_NewFloat64(quick_ctx_.context(), 3.11f);

    auto dict = lepus::Dictionary::Create();
    lepus::Value bool_temp = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_bool);
    dict->SetValue("key1", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str1));
    dict->SetValue("key2", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str2));
    dict->SetValue("key3", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_int64));
    dict->SetValue("key4", bool_temp);
    dict->SetValue("key5", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_int));
    dict->SetValue("key6", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_arr));
    dict->SetValue("key7", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_obj));
    dict->SetValue("key8", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_double));
    auto arr = lepus::CArray::Create();
    arr->push_back(MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str1));
    arr->push_back(MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_bool));
    dict->SetValue("key9", lepus::Value(arr));

    lepus::Value v1(dict);
    auto ret_val = v1.ToLepusValue(true);
    ASSERT_TRUE(ret_val.IsTable());
    ASSERT_TRUE(ret_val.GetLength() == 9);
    auto r1 = ret_val.GetProperty("key1");
    ASSERT_TRUE(r1.StdString() == "test_val");
    auto r2 = ret_val.GetProperty("key2");
    ASSERT_TRUE(r2.StdString() == "test_abc");
    auto r3 = ret_val.GetProperty("key3");
    ASSERT_TRUE(r3.Int64() == num);
    auto r4 = ret_val.GetProperty("key4");
    ASSERT_TRUE(r4.Bool());
    ASSERT_TRUE(!r4.IsJSBool());
    auto r5 = ret_val.GetProperty("key5");
    ASSERT_TRUE(r5.Int32() == 111);
    auto r6 = ret_val.GetProperty("key6");
    ASSERT_TRUE(r6.IsArray());
    auto r7 = ret_val.GetProperty("key7");
    ASSERT_TRUE(r7.IsTable());
    auto r8 = ret_val.GetProperty("key8");
    ASSERT_TRUE(r8.Double() == 3.11f);
    auto r9 = ret_val.GetProperty("key9");
    ASSERT_TRUE(r9.IsArray());
    ASSERT_TRUE(r9.GetLength() == 2);
    LEPUSRefCountHeader* p1 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
    ASSERT_TRUE(p1->ref_count == 1);
    LEPUSRefCountHeader* p2 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_arr);
    ASSERT_TRUE(p2->ref_count == 1);
    LEPUSRefCountHeader* p3 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_obj);
    ASSERT_TRUE(p3->ref_count == 1);

    LEPUSValue js_arr = LEPUS_NewArray(quick_ctx_.context());
    lepus::Value js_arr_val = MK_JS_LEPUS_VALUE(quick_ctx_.context(), js_arr);
    js_arr_val.SetProperty(0, v1);
    {
      lepus::Value ret_arr_val1 = js_arr_val.ToLepusValue();
      ASSERT_TRUE(ret_arr_val1.IsArray());
      lepus::Value element1 = ret_arr_val1.GetProperty(0);
      ASSERT_TRUE(element1.IsTable());
      ASSERT_TRUE(element1.GetLength() == 9);
      auto property1 = element1.GetProperty("key1");
      ASSERT_FALSE(property1.IsJSValue());
      ASSERT_TRUE(property1.StdString() == "test_val");
    }
    {
      lepus::Value ret_arr_val2 = js_arr_val.ToLepusValue(true);
      auto property1 = ret_arr_val2.GetProperty(0).GetProperty("key1");
      ASSERT_FALSE(property1.IsJSValue());
      ASSERT_TRUE(property1.StdString() == "test_val");
    }

    // free
    LEPUS_FreeValue(quick_ctx_.context(), val_str1);
    LEPUS_FreeValue(quick_ctx_.context(), val_str2);
    LEPUS_FreeValue(quick_ctx_.context(), val_int64);
    LEPUS_FreeValue(quick_ctx_.context(), val_bool);
    LEPUS_FreeValue(quick_ctx_.context(), val_arr);
    LEPUS_FreeValue(quick_ctx_.context(), val_obj);
    LEPUS_FreeValue(quick_ctx_.context(), val_double);
    LEPUS_FreeValue(quick_ctx_.context(), js_arr);
  }
  {
    LEPUSValue val_str1 =
        LEPUS_NewStringLen(quick_ctx_.context(), "test_val", 8);
    LEPUSValue val_str2 =
        LEPUS_NewStringLen(quick_ctx_.context(), "test_abc", 8);
    int64_t num = (int64_t)INT32_MAX + 3;
    LEPUSValue val_int64 = LEPUS_NewInt64(quick_ctx_.context(), num);
    LEPUSValue val_bool = LEPUS_NewBool(quick_ctx_.context(), true);
    LEPUSValue val_int = LEPUS_NewInt32(quick_ctx_.context(), 111);
    LEPUSValue val_arr = LEPUS_NewArray(quick_ctx_.context());
    LEPUSValue val_obj = LEPUS_NewObject(quick_ctx_.context());
    LEPUSValue val_double = LEPUS_NewFloat64(quick_ctx_.context(), 3.11f);

    auto dict = lepus::Dictionary::Create();
    dict->SetValue("key1", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str1));
    dict->SetValue("key2", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str2));
    dict->SetValue("key3", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_int64));
    dict->SetValue("key4", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_bool));
    dict->SetValue("key5", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_int));
    dict->SetValue("key6", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_arr));
    dict->SetValue("key7", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_obj));
    dict->SetValue("key8", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_double));
    auto arr = lepus::CArray::Create();
    arr->push_back(MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str1));
    arr->push_back(MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_bool));
    dict->SetValue("key9", lepus::Value(arr));

    lepus::Value v1(dict);
    auto ret_val = v1.ToLepusValue();
    ASSERT_TRUE(ret_val.IsTable());
    ASSERT_TRUE(ret_val.GetLength() == 9);
    auto r1 = ret_val.GetProperty("key1");
    ASSERT_TRUE(r1.StdString() == "test_val");
    auto r2 = ret_val.GetProperty("key2");
    ASSERT_TRUE(r2.StdString() == "test_abc");
    auto r3 = ret_val.GetProperty("key3");
    ASSERT_TRUE(r3.Int64() == num);
    auto r4 = ret_val.GetProperty("key4");
    ASSERT_TRUE(r4.Bool());
    auto r5 = ret_val.GetProperty("key5");
    ASSERT_TRUE(r5.Int32() == 111);
    auto r6 = ret_val.GetProperty("key6");
    ASSERT_TRUE(r6.IsArray());
    ASSERT_TRUE(r6.GetLength() == 0);
    auto r7 = ret_val.GetProperty("key7");
    ASSERT_TRUE(r7.IsTable());
    ASSERT_TRUE(r7.GetLength() == 0);
    auto r8 = ret_val.GetProperty("key8");
    ASSERT_TRUE(r8.Double() == 3.11f);
    auto r9 = ret_val.GetProperty("key9");
    ASSERT_TRUE(r9.IsArray());
    ASSERT_TRUE(r9.GetLength() == 2);
    LEPUSRefCountHeader* p1 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
    ASSERT_TRUE(p1->ref_count == 1);
    LEPUSRefCountHeader* p2 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_arr);
    ASSERT_TRUE(p2->ref_count == 1);
    LEPUSRefCountHeader* p3 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_obj);
    ASSERT_TRUE(p3->ref_count == 1);

    // free
    LEPUS_FreeValue(quick_ctx_.context(), val_str1);
    LEPUS_FreeValue(quick_ctx_.context(), val_str2);
    LEPUS_FreeValue(quick_ctx_.context(), val_int64);
    LEPUS_FreeValue(quick_ctx_.context(), val_bool);
    LEPUS_FreeValue(quick_ctx_.context(), val_arr);
    LEPUS_FreeValue(quick_ctx_.context(), val_obj);
    LEPUS_FreeValue(quick_ctx_.context(), val_double);
  }
}

TEST_F(LepusValueTest, LepusValueCloneValue) {
  LEPUSValue val_str1 = LEPUS_NewStringLen(quick_ctx_.context(), "test_val", 8);
  auto arr_ref = lepus::CArray::Create();
  auto dict_ref = lepus::Dictionary::Create();
  {
    auto dict = lepus::Dictionary::Create();
    dict->SetValue("key1", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str1));
    dict->SetValue("key2", lepus::Value());
    lepus::Value undefined;
    undefined.SetUndefined();
    dict->SetValue("key3", undefined);
    dict->SetValue("key4", lepus::Value((int32_t)10));
    dict->SetValue("key5", lepus::Value((uint32_t)11));
    dict->SetValue("key6", lepus::Value((int64_t)12));
    dict->SetValue("key7", lepus::Value((uint64_t)13));
    dict->SetValue("key8", lepus::Value(3.45f));
    dict->SetValue("key9", lepus::Value("string"));
    auto arr1 = lepus::CArray::Create();
    arr1->push_back(lepus::Value(false));
    arr1->push_back(lepus::Value("str"));
    dict->SetValue("key10", lepus::Value(arr1));
    auto dict1 = lepus::Dictionary::Create();
    dict1->SetValue("key1", lepus::Value("string"));
    dict->SetValue("key11", lepus::Value(dict1));

    LEPUSValue val_ref1 = lepus::LEPUSValueHelper::CreateLepusRef(
        quick_ctx_.context(), arr_ref.get(), Value_Array);
    dict->SetValue("key12", MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_ref1));
    LEPUSRefCountHeader* p_ref =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_ref1);
    ASSERT_TRUE(p_ref->ref_count == 1);

    LEPUSValue val_ref2 = lepus::LEPUSValueHelper::CreateLepusRef(
        quick_ctx_.context(), dict_ref.get(), Value_Table);
    lepus::Value id13 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_ref2);
    dict->SetValue("key13", id13);

    lepus::Value v1(dict);
    LEPUSRefCountHeader* p1 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
    ASSERT_TRUE(p1->ref_count == 2);

    auto ret1 = lepus::Value::Clone(v1, true);
    ASSERT_TRUE(ret1.IsTable());
    LEPUSRefCountHeader* p2 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
    ASSERT_TRUE(p2->ref_count == 2);
    auto r1 = ret1.GetProperty("key1");
    ASSERT_TRUE(r1.StdString() == "test_val");
    ASSERT_TRUE(!r1.IsJSString());
    auto r2 = ret1.GetProperty("key2");
    ASSERT_TRUE(r2.IsNil());
    auto r3 = ret1.GetProperty("key3");
    ASSERT_TRUE(r3.IsUndefined());
    auto r4 = ret1.GetProperty("key4");
    ASSERT_TRUE(r4.Int32() == 10);
    auto r5 = ret1.GetProperty("key5");
    ASSERT_TRUE(r5.UInt32() == 11);
    auto r6 = ret1.GetProperty("key6");
    ASSERT_TRUE(r6.Int64() == 12);
    auto r7 = ret1.GetProperty("key7");
    ASSERT_TRUE(r7.UInt64() == 13);
    auto r8 = ret1.GetProperty("key8");
    ASSERT_TRUE(r8.Double() == 3.45f);
    auto r9 = ret1.GetProperty("key9");
    ASSERT_TRUE(r9.StdString() == "string");
    auto r10 = ret1.GetProperty("key10");
    ASSERT_TRUE(r10.Array()->size() == 2);
    ASSERT_TRUE(r10.Array()->get(1).StdString() == "str");
    auto r11 = ret1.GetProperty("key11");
    ASSERT_TRUE(r11.Table()->size() == 1);
    auto r12 = ret1.GetProperty("key12");
    ASSERT_TRUE(r12.IsArray());
    auto r13 = ret1.GetProperty("key13");
    ASSERT_TRUE(r13.IsTable());
    ASSERT_TRUE(ret1.IsEqual(v1));

    auto ret2 = lepus::Value::Clone(v1, false);
    ASSERT_TRUE(ret2.GetLength() == 13);
    ASSERT_TRUE(ret2.IsEqual(v1));
    ret2.SetProperty("key14", lepus::Value("value14"));
    ASSERT_TRUE(ret2.GetLength() == 14);
    ASSERT_TRUE(v1.GetLength() == 13);
    ASSERT_TRUE(!ret2.GetProperty("key1").IsJSString());
    ASSERT_TRUE(!ret2.IsEqual(v1));

    auto ret3 = lepus::Value::ShallowCopy(v1, true);
    ASSERT_TRUE(ret3.IsEqual(v1));

    auto ret4 = lepus::Value::ShallowCopy(v1, false);
    ASSERT_TRUE(ret4.IsEqual(v1));

    LEPUS_FreeValue(quick_ctx_.context(), val_ref1);
    LEPUS_FreeValue(quick_ctx_.context(), val_ref2);
  }
  LEPUSRefCountHeader* p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
  ASSERT_TRUE(p->ref_count == 1);

  LEPUSValue val_str2 = LEPUS_NewStringLen(quick_ctx_.context(), "test_abc", 8);
  {
    lepus::Value v2 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str2);
    auto ret3 = lepus::Value::Clone(v2, true);
    ASSERT_TRUE(ret3.IsJSString());
    ASSERT_TRUE(ret3.IsEqual(v2));
    LEPUSRefCountHeader* p3 =
        (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str2);
    ASSERT_TRUE(p3->ref_count == 3);

    auto ret4 = lepus::Value::Clone(v2, false);
    ASSERT_TRUE(!ret4.IsJSString());
    ASSERT_TRUE(ret4.StdString() == "test_abc");
    ASSERT_TRUE(ret4.IsEqual(v2));
  }
  LEPUSRefCountHeader* p4 = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str2);
  ASSERT_TRUE(p4->ref_count == 1);
  {
    std::vector<LEPUSValue> values;
    values.reserve(10);
    for (int32_t i = 0; i < 10; i++) {
      values.emplace_back(LEPUS_NewInt32(quick_ctx_.context(), i));
    }
    LEPUSValue val_arr =
        LEPUS_NewArrayWithValue(quick_ctx_.context(), 10, values.data());
    lepus::Value v3 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_arr);
    auto ret5 = lepus::Value::Clone(v3, true);
    ASSERT_TRUE(!ret5.IsArray());
    ASSERT_TRUE(ret5.IsEqual(v3));

    auto ret6 = lepus::Value::Clone(v3, false);
    ASSERT_TRUE(ret6.IsArray());
    ASSERT_TRUE(ret6.IsEqual(v3));

    auto ret7 = lepus::Value::ShallowCopy(v3, true);
    ASSERT_TRUE(!ret7.IsArray());
    ASSERT_TRUE(ret7.IsEqual(v3));

    auto ret8 = lepus::Value::ShallowCopy(v3, false);
    ASSERT_TRUE(ret8.IsArray());
    ASSERT_TRUE(ret8.IsEqual(v3));

    LEPUS_FreeValue(quick_ctx_.context(), val_arr);
  }

  LEPUS_FreeValue(quick_ctx_.context(), val_str1);
  LEPUS_FreeValue(quick_ctx_.context(), val_str2);
}

TEST_F(LepusValueTest, LepusValueLepusRef) {
  auto arr_ref = lepus::CArray::Create();
  LEPUSValue val_ref1 = lepus::LEPUSValueHelper::CreateLepusRef(
      quick_ctx_.context(), arr_ref.get(), Value_Array);
  LEPUSRefCountHeader* p_ref =
      (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_ref1);
  ASSERT_TRUE(p_ref->ref_count == 1);
  LEPUS_FreeValue(quick_ctx_.context(), val_ref1);
}

TEST_F(LepusValueTest, LepusValueMarkConst) {
  LEPUSValue val_str1 =
      LEPUS_NewStringLen(quick_ctx_.context(), "test_abcd", 9);
  LEPUSValue val_int = LEPUS_NewInt32(quick_ctx_.context(), 111);
  lepus::Value v1 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str1);
  ASSERT_TRUE(!v1.MarkConst());
  lepus::Value v2 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_int);
  ASSERT_TRUE(v2.MarkConst());
  lepus::Value v3(10);
  ASSERT_TRUE(v3.MarkConst());
  lepus::Value v4(9.5f);
  ASSERT_TRUE(v4.MarkConst());
  auto arr = lepus::CArray::Create();
  lepus::Value v5(arr);
  ASSERT_TRUE(v5.MarkConst());
  lepus::Value v6(lepus::Dictionary::Create());
  ASSERT_TRUE(v6.MarkConst());

  LEPUS_FreeValue(quick_ctx_.context(), val_str1);
  LEPUS_FreeValue(quick_ctx_.context(), val_int);
}

TEST_F(LepusValueTest, LepusValuePrint) {
  auto dict = lepus::Dictionary::Create();
  dict->SetValue("key1", lepus::Value((int32_t)101));
  dict->SetValue("key2", lepus::Value((int64_t)2001));
  dict->SetValue("key3", lepus::Value());
  dict->SetValue("key4", lepus::Value(false));
  dict->SetValue("key5", lepus::Value("testing"));
  lepus::Value v1(dict);
  v1.Print();

  auto arr1 = lepus::CArray::Create();
  arr1->push_back(lepus::Value((int32_t)101));
  arr1->push_back(lepus::Value((int64_t)2001));
  arr1->push_back(lepus::Value());
  arr1->push_back(lepus::Value(false));
  arr1->push_back(lepus::Value("testing"));
  lepus::Value undefined;
  undefined.SetUndefined();
  arr1->push_back(undefined);
  lepus::Value v2(arr1);
  std::ostringstream s;
  v2.PrintValue(s);
  ASSERT_TRUE(s.str() == "[101,2001,null,false,testing,undefined]");
}

TEST_F(LepusValueTest, LepusValueMergeValue) {
  auto dict1 = lepus::Dictionary::Create();
  dict1->SetValue("key1", lepus::Value((int32_t)101));
  dict1->SetValue("key2", lepus::Value((int64_t)2001));
  dict1->SetValue("key3", lepus::Value());
  dict1->SetValue("key4", lepus::Value(false));
  dict1->SetValue("key5", lepus::Value("testing"));
  lepus::Value v1(dict1);

  auto dict2 = lepus::Dictionary::Create();
  dict2->SetValue("key6", lepus::Value((int32_t)102));
  dict2->SetValue("key7", lepus::Value((int64_t)2002));
  dict2->SetValue("key8", lepus::Value());
  dict2->SetValue("key9", lepus::Value(true));
  dict2->SetValue("key10", lepus::Value("testing111"));
  lepus::Value v2(dict2);

  auto dict3 = lepus::Dictionary::Create();
  dict3->SetValue("key1", lepus::Value((int32_t)101));
  dict3->SetValue("key2", lepus::Value((int64_t)2001));
  dict3->SetValue("key3", lepus::Value());
  dict3->SetValue("key4", lepus::Value(false));
  dict3->SetValue("key5", lepus::Value("testing"));
  dict3->SetValue("key6", lepus::Value((int32_t)102));
  dict3->SetValue("key7", lepus::Value((int64_t)2002));
  dict3->SetValue("key8", lepus::Value());
  dict3->SetValue("key9", lepus::Value(true));
  dict3->SetValue("key10", lepus::Value("testing111"));
  lepus::Value v3(dict3);

  lepus::Value::MergeValue(v1, v2);
  ASSERT_TRUE(v1.IsEqual(v3));
}

TEST_F(LepusValueTest, LepusValueToJSValue) {
  {
    auto dict1 = lepus::Dictionary::Create();
    dict1->SetValue("key1", lepus::Value((int32_t)101));
    dict1->SetValue("key2", lepus::Value((int64_t)2001));
    dict1->SetValue("key3", lepus::Value());
    dict1->SetValue("key4", lepus::Value(false));
    dict1->SetValue("key5", lepus::Value("hello world"));
    auto arr1 = lepus::CArray::Create();
    arr1->push_back(lepus::Value((int32_t)1033));
    arr1->push_back(lepus::Value((int64_t)200111));
    arr1->push_back(lepus::Value(3.56f));
    arr1->push_back(lepus::Value(true));
    arr1->push_back(lepus::Value("testing_abc"));
    dict1->SetValue("key6", lepus::Value(arr1));
    lepus::Value v1(dict1);

    LEPUSValue ret1 =
        lepus::LEPUSValueHelper::ToJsValue(quick_ctx_.context(), v1, true);
    bool is_obj = lepus::LEPUSValueHelper::IsJsObject(ret1);
    ASSERT_TRUE(is_obj);
    LEPUSValue ret2 =
        lepus::LEPUSValueHelper::ToJsValue(quick_ctx_.context(), v1, false);
    bool is_lepus_obj = lepus::LEPUSValueHelper::IsLepusTable(ret2);
    ASSERT_TRUE(is_lepus_obj);

    lepus::Value v2 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), ret1);
    ASSERT_TRUE(v2.GetLength() == 6);
    auto r1 = v2.GetProperty("key6");
    ASSERT_TRUE(r1.IsJSArray());
    auto ret3 = v2.ToLepusValue(true);
    auto r2 = v2.GetProperty("key6");
    ASSERT_TRUE(r2.IsArray());
    ASSERT_TRUE(v2.IsEqual(ret3));
    auto r3 = v2.GetProperty("key1");
    ASSERT_TRUE(r3.Int32() == 101);
    auto r4 = v2.GetProperty("key2");
    ASSERT_TRUE(r4.Int32() == 2001);
    auto r5 = v2.GetProperty("key3");
    ASSERT_TRUE(r5.IsNil());
    auto r6 = v2.GetProperty("key4");
    ASSERT_TRUE(!r6.Bool());
    auto r7 = v2.GetProperty("key5");
    ASSERT_TRUE(r7.StdString() == "hello world");

    lepus::Value v3 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), ret2);
    ASSERT_TRUE(v3.GetLength() == 6);
    ASSERT_TRUE(v3.IsTable());
    auto r8 = v3.GetProperty("key5");
    ASSERT_TRUE(r8.StdString() == "hello world");

    LEPUS_FreeValue(quick_ctx_.context(), ret1);
    LEPUS_FreeValue(quick_ctx_.context(), ret2);
  }
  {
    lepus::Value v4("hello lynx");
    LEPUSValue ret3 =
        lepus::LEPUSValueHelper::ToJsValue(quick_ctx_.context(), v4, true);
    bool is_str = lepus::LEPUSValueHelper::IsJSString(ret3);
    ASSERT_TRUE(is_str);

    LEPUS_FreeValue(quick_ctx_.context(), ret3);
  }
  {
    auto arr1 = lepus::CArray::Create();
    arr1->push_back(lepus::Value((int32_t)101));
    arr1->push_back(lepus::Value((int64_t)2001));
    arr1->push_back(lepus::Value());
    arr1->push_back(lepus::Value(false));
    arr1->push_back(lepus::Value("testing"));
    lepus::Value undefined;
    undefined.SetUndefined();
    arr1->push_back(undefined);
    lepus::Value v5(arr1);
    LEPUSValue ret4 =
        lepus::LEPUSValueHelper::ToJsValue(quick_ctx_.context(), v5, false);
    auto str = lepus::LEPUSValueHelper::LepusRefToStdString(
        quick_ctx_.context(), ret4);
    ASSERT_TRUE(str == "[101,2001,null,false,testing,undefined]");
    lepus::Value v6 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), ret4);
    auto ret5 = v6.ToLepusValue();
    ASSERT_TRUE(ret5.IsArray());

    fml::RefPtr<lepus::RefCounted> ref_counted = lepus::LEPUSObject::Create();
    lepus::Value v7(ref_counted);
    ASSERT_TRUE(v7.IsRefCounted());
    LEPUSValue ret6 =
        lepus::LEPUSValueHelper::ToJsValue(quick_ctx_.context(), v7, false);
    lepus::Value v8 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), ret6);
    auto ret7 = v8.ToLepusValue();
    ASSERT_TRUE(ret7.IsRefCounted());

    LEPUSValue ret8 =
        lepus::LEPUSValueHelper::ToJsValue(quick_ctx_.context(), v7, true);
    lepus::Value v9 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), ret8);
    auto ret9 = v9.ToLepusValue();
    ASSERT_TRUE(ret9.IsObject());

    LEPUS_FreeValue(quick_ctx_.context(), ret4);
    LEPUS_FreeValue(quick_ctx_.context(), ret6);
    LEPUS_FreeValue(quick_ctx_.context(), ret8);
  }
}

TEST_F(LepusValueTest, LepusValueLepusRefStorage) {
  LEPUSValue val_str1 =
      LEPUS_NewStringLen(quick_ctx_.context(), "test_1234", 9);
  LEPUSValue val_map1 = LEPUS_NewObject(quick_ctx_.context());
  lepus::Dictionary* dict_ptr = nullptr;
  lepus::CArray* array_ptr = nullptr;

  {
    auto dict1 = lepus::Dictionary::Create();
    auto dict2 = lepus::Dictionary::Create();
    dict_ptr = dict2.get();
    auto array1 = lepus::CArray::Create();
    array1->push_back(lepus::Value("test"));
    array_ptr = array1.get();
    {
      lepus::Value v_dict(dict1);
      lepus::Value v1 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_str1);
      v_dict.SetProperty("key1", std::move(v1));
      lepus::Value v2 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), val_map1);
      dict2->SetValue("key1", lepus::Value(true));
      dict2->SetValue("key2", lepus::Value("string"));
      dict2->SetValue("key3", lepus::Value(array1));
      lepus::Value v3(dict2);
      v2.SetProperty("key", v3);
      v_dict.SetProperty("key2", v2);
      auto ret1 = v_dict.GetProperty("key2");
      ASSERT_TRUE(ret1.IsJSTable());
      auto ret2 = ret1.GetProperty("key");
      ASSERT_TRUE(ret2.IsTable());

      LEPUSValue ret3 =
          lepus::LEPUSValueHelper::ToJsValue(quick_ctx_.context(), v_dict);
      lepus::Value v4 = MK_JS_LEPUS_VALUE(quick_ctx_.context(), ret3);
      auto ret4 = v4.ToLepusValue();
      ASSERT_TRUE(ret4 == v_dict);
      LEPUS_FreeValue(quick_ctx_.context(), ret3);
    }
    ASSERT_TRUE(dict1->HasOneRef());
  }

  ASSERT_TRUE(dict_ptr->HasOneRef());
  ASSERT_TRUE(array_ptr->HasOneRef());
  LEPUSRefCountHeader* p_ref1 =
      (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str1);
  ASSERT_TRUE(p_ref1->ref_count == 1);
  LEPUS_FreeValue(quick_ctx_.context(), val_str1);
  LEPUSRefCountHeader* p_ref2 =
      (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_map1);
  ASSERT_TRUE(p_ref2->ref_count == 1);
  LEPUS_FreeValue(quick_ctx_.context(), val_map1);
}

}  // namespace test
}  // namespace lepus
}  // namespace lynx
