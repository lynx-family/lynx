// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus_context/restricted_value.h"

#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/value/base_value.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "core/runtime/lepus_context/function.h"
#include "core/runtime/lepus_context/js_object.h"
#include "core/runtime/lepus_context/jsvalue_helper.h"
#include "core/runtime/lepus_context/lepus_date.h"
#include "core/runtime/lepus_context/regexp.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace lepus {
namespace test {

namespace {
static RestrictedValue TestCFunction(VMContext* ctx) {
  return RestrictedValue("test");
}
}  // namespace

class RestrictedValueTest : public ::testing::Test {
 protected:
  RestrictedValueTest() = default;
  ~RestrictedValueTest() = default;
};

TEST_F(RestrictedValueTest, RestrictedValueNull) {
  RestrictedValue v1;
  ASSERT_TRUE(v1.IsNil());
  ASSERT_TRUE(!v1.IsReference());
  ASSERT_TRUE(v1.IsFalse());
  ASSERT_TRUE(v1.Type() == Value_Nil);
  RestrictedValue v5;
  v5.SetNil();
  ASSERT_TRUE(v5.IsNil());
  ASSERT_TRUE(v5.IsEmpty());

  RestrictedValue v6;
  v6.SetUndefined();
  ASSERT_TRUE(v6.IsUndefined());
  ASSERT_TRUE(v6.IsEmpty());
  ASSERT_TRUE(v6.IsFalse());
  ASSERT_TRUE(v6.Type() == Value_Undefined);

  RestrictedValue v7(RestrictedValue::kCreateAsUndefinedTag);
  ASSERT_TRUE(v7.IsUndefined());
  ASSERT_TRUE(v7.IsEmpty());
  ASSERT_TRUE(v7.IsFalse());
  ASSERT_TRUE(v7.Type() == Value_Undefined);
}

TEST_F(RestrictedValueTest, RestrictedValueNumber) {
  {
    RestrictedValue v1(RestrictedValue::kCreateAsNanTag);
    ASSERT_TRUE(v1.IsNaN());
    ASSERT_TRUE(v1.NaN());
    ASSERT_TRUE(v1.Type() == Value_NaN);
    RestrictedValue v2;
    v2.SetNan(true);
    ASSERT_TRUE(v2.IsNaN());
    ASSERT_TRUE(v2.NaN());
    ASSERT_TRUE(v2.IsFalse());
  }
  {
    RestrictedValue v3((int32_t)10);
    ASSERT_TRUE(v3.IsInt32());
    ASSERT_TRUE(v3.IsNumber());
    ASSERT_TRUE(v3.Int32() == 10);
    ASSERT_TRUE(v3.Type() == Value_Int32);
    v3.SetNumber((uint32_t)100);
    ASSERT_TRUE(v3.IsUInt32());
    ASSERT_TRUE(v3.UInt32() == 100);
    ASSERT_TRUE(v3.Type() == Value_UInt32);
    RestrictedValue v4((uint32_t)50);
    ASSERT_TRUE(v4.IsUInt32());
    ASSERT_TRUE(v4.IsNumber());
    ASSERT_TRUE(v4.UInt32() == 50);
    v4.SetNumber((int32_t)101);
    ASSERT_TRUE(v4.IsInt32());
    ASSERT_TRUE(v4.Int32() == 101);
    int64_t num1 = (int64_t)INT32_MAX + 1;
    RestrictedValue v5(num1);
    ASSERT_TRUE(v5.IsInt64());
    ASSERT_TRUE(v5.IsNumber());
    ASSERT_TRUE(v5.Int64() == num1);
    ASSERT_TRUE(v5.Type() == Value_Int64);
    v5.SetNumber((int32_t)101);
    ASSERT_TRUE(v5.IsInt32());
    ASSERT_TRUE(v5.Int32() == 101);
    uint64_t num2 = (uint64_t)INT32_MAX + 10;
    RestrictedValue v6(num2);
    ASSERT_TRUE(v6.IsUInt64());
    ASSERT_TRUE(v6.IsNumber());
    ASSERT_TRUE(v6.UInt64() == num2);
    ASSERT_TRUE(v6.Type() == Value_UInt64);
    v6.SetNumber((int32_t)101);
    ASSERT_TRUE(v6.IsInt32());
    ASSERT_TRUE(v6.Int32() == 101);
    RestrictedValue v7(3.14f);
    ASSERT_TRUE(v7.IsDouble());
    ASSERT_TRUE(v7.IsNumber());
    ASSERT_TRUE(v7.Double() == 3.14f);
    ASSERT_TRUE(v7.Type() == Value_Double);
    v7.SetNumber((int32_t)101);
    ASSERT_TRUE(v7.IsInt32());
    ASSERT_TRUE(v7.Int32() == 101);
    RestrictedValue v8((uint8_t)3);
    ASSERT_TRUE(v8.IsUInt32());
    ASSERT_TRUE(v8.UInt32() == 3);
  }
}

TEST_F(RestrictedValueTest, RestrictedValueString) {
  {
    base::String s1("test");
    RestrictedValue v1(s1);
    ASSERT_TRUE(v1.IsString());
    ASSERT_TRUE(v1.StdString() == "test");
    ASSERT_TRUE(v1.GetLength() == 4);
    ASSERT_TRUE(v1.IsReference());
    ASSERT_TRUE(v1.Type() == Value_String);

    RestrictedValue b1(true);
    ASSERT_TRUE(b1.IsBool());
    ASSERT_TRUE(b1.Type() == Value_Bool);
    ASSERT_TRUE(b1.StdString() == "true");

    RestrictedValue v3(std::move(v1));
    ASSERT_TRUE(v3.IsString());
    ASSERT_TRUE(v3.StdString() == "test");

    base::String s2("test2");
    RestrictedValue v6(std::move(s2));
    ASSERT_TRUE(v6.IsString());
    ASSERT_TRUE(v6.StdString() == "test2");
  }

  {
    RestrictedValue v7("abcd");
    ASSERT_TRUE(v7.IsString());
    ASSERT_TRUE(v7.StdString() == "abcd");
    RestrictedValue v8(v7);
    ASSERT_TRUE(v8.IsString());
    ASSERT_TRUE(v8.StdString() == "abcd");
    auto str = v8.String();
    ASSERT_TRUE(str.str() == "abcd");
    auto str1 = RestrictedValue(v8).String();
    ASSERT_TRUE(str1.str() == "abcd");
  }

  {
    RestrictedValue v9;
    base::String s3("test");
    v9.SetString(s3);
    ASSERT_TRUE(v9.IsString());
    ASSERT_TRUE(v9.StdString() == "test");

    RestrictedValue v10;
    v10.SetString(base::String("aaa"));
    ASSERT_TRUE(v10.IsString());
    ASSERT_TRUE(v10.StdString() == "aaa");

    auto r2 = v9.GetProperty(2);
    ASSERT_TRUE(r2.StdString() == "s");

    RestrictedValue v11("");
    ASSERT_TRUE(v11.IsFalse());
  }
}

TEST_F(RestrictedValueTest, RestrictedValueArray) {
  auto arr1 = lepus::CArray::Create();
  arr1->push_back(RestrictedValue((int32_t)101));
  arr1->push_back(RestrictedValue((int64_t)2001));
  arr1->push_back(RestrictedValue(5.645f));
  arr1->push_back(RestrictedValue(false));
  arr1->push_back(RestrictedValue("testing"));
  RestrictedValue v1(arr1);
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

  v1.SetProperty(5, RestrictedValue(true));
  auto r5 = v1.GetProperty(5);
  ASSERT_TRUE(r5.Bool() == true);
  RestrictedValue id6("666");
  v1.SetProperty(6, id6);
  auto r6 = v1.GetProperty(6);
  ASSERT_TRUE(r6.StdString() == "666");
  ASSERT_TRUE(v1.GetLength() == 7);
  ASSERT_TRUE(v1.Array()->size() == 7);
}

TEST_F(RestrictedValueTest, RestrictedValueMap) {
  auto dict = lepus::Dictionary::Create();
  dict->SetValue("key1", RestrictedValue((int32_t)101));
  dict->SetValue("key2", RestrictedValue((int64_t)2001));
  dict->SetValue("key3", RestrictedValue(5.645f));
  dict->SetValue("key4", RestrictedValue(false));
  dict->SetValue("key5", RestrictedValue("testing"));
  RestrictedValue v1(dict);
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

  v1.SetProperty("key6", RestrictedValue(true));
  auto r5 = v1.GetProperty("key6");
  ASSERT_TRUE(r5.Bool() == true);
  RestrictedValue id6("666");
  v1.SetProperty("key7", id6);
  auto r6 = v1.GetProperty("key7");
  ASSERT_TRUE(r6.StdString() == "666");
  RestrictedValue id7("abc");
  base::String key8("key8");
  v1.SetProperty(key8, id7);
  auto r7 = v1.GetProperty("key8");
  ASSERT_TRUE(r7.StdString() == "abc");
  ASSERT_TRUE(v1.GetLength() == 8);
  ASSERT_TRUE(v1.Table()->size() == 8);
}

TEST_F(RestrictedValueTest, RestrictedValueArrayBuffer) {
  auto buffer1 = lepus::ByteArray::Create();
  RestrictedValue v1(buffer1);
  ASSERT_TRUE(v1.IsByteArray());
  auto buffer2 = lepus::ByteArray::Create();
  RestrictedValue v2(std::move(buffer2));
  ASSERT_TRUE(v2.IsByteArray());
  ASSERT_TRUE(v2.ByteArray());
  ASSERT_TRUE(v2.Type() == Value_ByteArray);
}

TEST_F(RestrictedValueTest, RestrictedValueRefCounted) {
  {
    auto js_object = lepus::LEPUSObject::Create();
    fml::RefPtr<lepus::RefCounted> ref_counted = js_object;
    RestrictedValue v1(ref_counted);
    ASSERT_TRUE(v1.IsRefCounted());
    RestrictedValue v2;
    v2.SetRefCounted(ref_counted);
    ASSERT_TRUE(v2.IsRefCounted());
    RestrictedValue v3;
    v3.SetRefCounted(std::move(ref_counted));
    ASSERT_TRUE(v3.IsRefCounted());
  }
  {
    auto s1 = lepus::Closure::Create(lepus::Function::Create());
    RestrictedValue v1(s1);
    ASSERT_TRUE(v1.IsClosure());
    RestrictedValue v2(std::move(s1));
    ASSERT_TRUE(v2.IsClosure());
    RestrictedValue v3(std::move(v2));
    ASSERT_TRUE(v3.IsClosure());
    auto s2 = lepus::Closure::Create(lepus::Function::Create());
    RestrictedValue v4;
    v4.SetRefCounted(s2);
    ASSERT_TRUE(v4.IsClosure());
    RestrictedValue v5;
    v5.SetRefCounted(lepus::Closure::Create(lepus::Function::Create()));
    ASSERT_TRUE(v5.IsClosure());
    ASSERT_TRUE(v5.Type() == Value_Closure);
    auto r1 = v5.RefCounted();
    ASSERT_TRUE(r1);
  }
  {
    auto s1 = lepus::CDate::Create();
    RestrictedValue v1(s1);
    ASSERT_TRUE(v1.IsCDate());
    RestrictedValue v2(std::move(s1));
    ASSERT_TRUE(v2.IsCDate());
    RestrictedValue v3(std::move(v2));
    ASSERT_TRUE(v3.IsCDate());
    auto s2 = lepus::CDate::Create();
    RestrictedValue v4;
    v4.SetRefCounted(s2);
    ASSERT_TRUE(v4.IsCDate());
    RestrictedValue v5;
    v5.SetRefCounted(lepus::CDate::Create());
    ASSERT_TRUE(v5.IsCDate());
    ASSERT_TRUE(v5.Type() == Value_CDate);
    auto r1 = v5.RefCounted();
    ASSERT_TRUE(r1);
  }
  {
    auto s1 = lepus::RegExp::Create();
    RestrictedValue v1(s1);
    ASSERT_TRUE(v1.IsRegExp());
    RestrictedValue v2(std::move(s1));
    ASSERT_TRUE(v2.IsRegExp());
    RestrictedValue v3(std::move(v2));
    ASSERT_TRUE(v3.IsRegExp());
    auto s2 = lepus::RegExp::Create();
    RestrictedValue v4;
    v4.SetRefCounted(s2);
    ASSERT_TRUE(v4.IsRegExp());
    RestrictedValue v5;
    v5.SetRefCounted(lepus::RegExp::Create());
    ASSERT_TRUE(v5.IsRegExp());
    ASSERT_TRUE(v5.Type() == Value_RegExp);
    auto r1 = v5.RefCounted();
    ASSERT_TRUE(r1);
  }
}

TEST_F(RestrictedValueTest, RestrictedValuePointer) {
  {
    int* a = new int;
    *a = 10;
    RestrictedValue v1(a);
    ASSERT_TRUE(v1.IsCPointer());
    ASSERT_TRUE(v1.Type() == Value_CPointer);
    int* b = reinterpret_cast<int*>(v1.CPoint());
    ASSERT_TRUE(*a == *b);
  }

  {
    RestrictedValue v3(&TestCFunction);
    ASSERT_TRUE(v3.IsCFunction());
    ASSERT_TRUE(v3.Type() == Value_CFunction);
    ASSERT_TRUE(v3.GetCFunctionType() == CFunctionType_Builtin);
    ASSERT_TRUE(!v3.IsReference());
    auto func = v3.FunctionBuiltin();
    auto ret = func(nullptr);
    ASSERT_TRUE(ret.IsString());
  }
}

TEST_F(RestrictedValueTest, RestrictedValuePrint) {
  auto dict = lepus::Dictionary::Create();
  dict->SetValue("key1", RestrictedValue((int32_t)101));
  dict->SetValue("key2", RestrictedValue((int64_t)2001));
  dict->SetValue("key3", RestrictedValue());
  dict->SetValue("key4", RestrictedValue(false));
  dict->SetValue("key5", RestrictedValue("testing"));
  Value v1(dict);
  v1.Print();

  auto arr1 = lepus::CArray::Create();
  arr1->push_back(RestrictedValue((int32_t)101));
  arr1->push_back(RestrictedValue((int64_t)2001));
  arr1->push_back(RestrictedValue());
  arr1->push_back(RestrictedValue(false));
  arr1->push_back(RestrictedValue("testing"));
  RestrictedValue undefined;
  undefined.SetUndefined();
  arr1->push_back(undefined);
  Value v2(arr1);
  std::ostringstream s;
  v2.PrintValue(s);
  ASSERT_TRUE(s.str() == "[101,2001,null,false,testing,undefined]");
}

}  // namespace test
}  // namespace lepus
}  // namespace lynx
