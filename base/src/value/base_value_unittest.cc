// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/value/base_value.h"

#include <functional>
#include <unordered_set>

#include "base/include/value/array.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/lynx_value_api.h"
#include "base/include/value/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

class BaseValueTest : public ::testing::Test {
 protected:
  BaseValueTest() = default;
  ~BaseValueTest() override = default;

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(BaseValueTest, BaseValueNull) {
  lepus::Value v1;
  ASSERT_TRUE(v1.IsNil());
  ASSERT_TRUE(!v1.IsReference());
  ASSERT_TRUE(v1.IsFalse());
  ASSERT_TRUE(v1.Type() == lepus::Value_Nil);
  lepus::Value v2;
  v2.SetNil();
  ASSERT_TRUE(v2.IsNil());
  ASSERT_TRUE(v2.IsEmpty());

  lepus::Value v3;
  v3.SetUndefined();
  ASSERT_TRUE(v3.IsUndefined());
  ASSERT_TRUE(v3.IsEmpty());
  ASSERT_TRUE(v3.IsFalse());
  ASSERT_TRUE(v3.Type() == lepus::Value_Undefined);
}

TEST_F(BaseValueTest, BaseValueNumber) {
  {
    lepus::Value v1(true, true);
    ASSERT_TRUE(v1.IsNaN());
    ASSERT_TRUE(v1.NaN());
    ASSERT_TRUE(v1.Type() == lepus::Value_NaN);
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
    ASSERT_TRUE(v3.Type() == lepus::Value_Int32);
    v3.SetNumber((uint32_t)100);
    ASSERT_TRUE(v3.IsUInt32());
    ASSERT_TRUE(v3.UInt32() == 100);
    ASSERT_TRUE(v3.Type() == lepus::Value_UInt32);
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
    ASSERT_TRUE(v5.Type() == lepus::Value_Int64);
    v5.SetNumber((int32_t)101);
    ASSERT_TRUE(v5.IsInt32());
    ASSERT_TRUE(v5.Int32() == 101);
    uint64_t num2 = (uint64_t)INT32_MAX + 10;
    lepus::Value v6(num2);
    ASSERT_TRUE(v6.IsUInt64());
    ASSERT_TRUE(v6.IsNumber());
    ASSERT_TRUE(v6.UInt64() == num2);
    ASSERT_TRUE(v6.Type() == lepus::Value_UInt64);
    v6.SetNumber((int32_t)101);
    ASSERT_TRUE(v6.IsInt32());
    ASSERT_TRUE(v6.Int32() == 101);
    lepus::Value v7(3.14f);
    ASSERT_TRUE(v7.IsDouble());
    ASSERT_TRUE(v7.IsNumber());
    ASSERT_TRUE(v7.Double() == 3.14f);
    ASSERT_TRUE(v7.Type() == lepus::Value_Double);
    v7.SetNumber((int32_t)101);
    ASSERT_TRUE(v7.IsInt32());
    ASSERT_TRUE(v7.Int32() == 101);
    lepus::Value v8((uint8_t)3);
    ASSERT_TRUE(v8.IsUInt32());
    ASSERT_TRUE(v8.UInt32() == 3);
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

TEST_F(BaseValueTest, BaseValueString) {
  {
    base::String s1("test");
    lepus::Value v1(s1);
    ASSERT_TRUE(v1.IsString());
    ASSERT_TRUE(v1.StdString() == "test");
    ASSERT_TRUE(v1.GetLength() == 4);
    ASSERT_TRUE(v1.IsReference());
    ASSERT_TRUE(v1.Type() == lepus::Value_String);
    lepus::Value b1(true);
    ASSERT_TRUE(b1.IsBool());
    ASSERT_TRUE(b1.Type() == lepus::Value_Bool);
    ASSERT_TRUE(b1.StdString() == "true");
    base::String s2("test2");
    lepus::Value v2(std::move(s2));
    ASSERT_TRUE(v2.IsString());
    ASSERT_TRUE(v2.StdString() == "test2");
  }
  {
    lepus::Value v3("abcd");
    ASSERT_TRUE(v3.IsString());
    ASSERT_TRUE(v3.StdString() == "abcd");
    lepus::Value v4(v3);
    ASSERT_TRUE(v4.IsString());
    ASSERT_TRUE(v4.StdString() == "abcd");
    auto str = v4.String();
    ASSERT_TRUE(str.str() == "abcd");
    auto str1 = lepus::Value(v4).String();
    ASSERT_TRUE(str1.str() == "abcd");
  }
  {
    lepus::Value v5;
    base::String s3("test");
    v5.SetString(s3);
    ASSERT_TRUE(v5.IsString());
    ASSERT_TRUE(v5.StdString() == "test");
    ASSERT_TRUE(v5.ToString() == "test");
    lepus::Value v6;
    v6.SetString(base::String("aaa"));
    ASSERT_TRUE(v6.IsString());
    ASSERT_TRUE(v6.StdString() == "aaa");
    auto r2 = v5.GetProperty(2);
    ASSERT_TRUE(r2.StdString() == "s");
    lepus::Value v7("");
    ASSERT_TRUE(v7.IsFalse());
  }
}

TEST_F(BaseValueTest, BaseValueArray) {
  auto arr1 = lepus::CArray::Create();
  arr1->push_back(lepus::Value((int32_t)101));
  arr1->push_back(lepus::Value((int64_t)2001));
  arr1->push_back(lepus::Value(5.645f));
  arr1->push_back(lepus::Value(false));
  arr1->push_back(lepus::Value("testing"));
  lepus::Value v1(arr1);
  ASSERT_TRUE(v1.Type() == lepus::Value_Array);
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

TEST_F(BaseValueTest, BaseValueMap) {
  auto dict = lepus::Dictionary::Create();
  dict->SetValue("key1", lepus::Value((int32_t)101));
  dict->SetValue("key2", lepus::Value((int64_t)2001));
  dict->SetValue("key3", lepus::Value(5.645f));
  dict->SetValue("key4", lepus::Value(false));
  dict->SetValue("key5", lepus::Value("testing"));
  lepus::Value v1(dict);
  ASSERT_TRUE(v1.Type() == lepus::Value_Table);
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

  for (auto& it : *(dict.get())) {
    auto key = it.first.str();
    if (key == "key8") {
      ASSERT_TRUE(it.second.StdString() == "abc");
    }
  }
}

TEST_F(BaseValueTest, BaseValueArrayBuffer) {
  auto buffer1 = lepus::ByteArray::Create();
  lepus::Value v1(buffer1);
  ASSERT_TRUE(v1.IsByteArray());
  auto buffer2 = lepus::ByteArray::Create();
  lepus::Value v2(std::move(buffer2));
  ASSERT_TRUE(v2.IsByteArray());
  ASSERT_TRUE(v2.ByteArray());
  ASSERT_TRUE(v2.Type() == lepus::Value_ByteArray);
}

TEST_F(BaseValueTest, BaseValuePointer) {
  int* a = new int;
  *a = 10;
  lepus::Value v1(a);
  ASSERT_TRUE(v1.IsCPointer());
  ASSERT_TRUE(v1.Type() == lepus::Value_CPointer);
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

TEST_F(BaseValueTest, BaseValueCloneValue) {
  auto arr_ref = lepus::CArray::Create();
  auto dict_ref = lepus::Dictionary::Create();
  auto dict = lepus::Dictionary::Create();
  dict->SetValue("key1", lepus::Value("test_val"));
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
  lepus::Value v1(dict);
  auto ret1 = lepus::Value::Clone(v1, true);
  ASSERT_TRUE(ret1.IsTable());
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
  ASSERT_TRUE(ret1.IsEqual(v1));
  auto ret2 = lepus::Value::Clone(v1, false);
  ASSERT_TRUE(ret2.GetLength() == 11);
  ASSERT_TRUE(ret2.IsEqual(v1));
  ret2.SetProperty("key12", lepus::Value("value12"));
  ASSERT_TRUE(ret2.GetLength() == 12);
  ASSERT_TRUE(v1.GetLength() == 11);
  ASSERT_TRUE(!ret2.GetProperty("key1").IsJSString());
  ASSERT_TRUE(!ret2.IsEqual(v1));
  auto ret3 = lepus::Value::ShallowCopy(v1, true);
  ASSERT_TRUE(ret3.IsEqual(v1));
  auto ret4 = lepus::Value::ShallowCopy(v1, false);
  ASSERT_TRUE(ret4.IsEqual(v1));
}

TEST_F(BaseValueTest, Dictionary) {
  auto dict = lepus::Dictionary::Create();
  ASSERT_TRUE(dict->using_small_map());
  ASSERT_TRUE(dict->empty());
  ASSERT_TRUE(dict->size() == 0);

  {
    auto dict = lepus::Dictionary::Create();
    for (int i = 0; i < (int)lepus::Dictionary::kSmallMapMaximumSize; i++) {
      dict->SetValue(String(std::to_string(i)), std::to_string(i));
    }
    ASSERT_TRUE(dict->using_small_map());
    // Dictionary iterates and set value from self. No change and no transfer.
    for (auto it = dict->begin(); it != dict->end(); it++) {
      dict->SetValue(it->first, it->second);
    }
    ASSERT_TRUE(dict->using_small_map());
    for (int i = 0; i < (int)lepus::Dictionary::kSmallMapMaximumSize; i++) {
      ASSERT_EQ(dict->GetValue(String(std::to_string(i))).StdString(),
                std::to_string(i));
    }
    auto* value_ptr =
        dict->SetValue(
                String(std::to_string(lepus::Dictionary::kSmallMapMaximumSize)),
                "asdf")
            .get();
    ASSERT_EQ(value_ptr,
              dict->GetValue(String(std::to_string(
                                 lepus::Dictionary::kSmallMapMaximumSize)))
                  .get());
    ASSERT_FALSE(dict->using_small_map());
    for (int i = 0; i < (int)lepus::Dictionary::kSmallMapMaximumSize; i++) {
      ASSERT_EQ(dict->GetValue(String(std::to_string(i))).StdString(),
                std::to_string(i));
    }
  }

  std::unordered_set<std::string> keys;
  for (int i = 0; i < (int)lepus::Dictionary::kSmallMapMaximumSize; i++) {
    dict->SetValue(String(std::to_string(i)), std::to_string(i));
    keys.insert(std::to_string(i));
  }
  ASSERT_TRUE(dict->size() == lepus::Dictionary::kSmallMapMaximumSize);
  ASSERT_TRUE(dict->using_small_map());

  for (int i = 0; i < (int)lepus::Dictionary::kSmallMapMaximumSize; i++) {
    auto key = String(std::to_string(i));
    ASSERT_TRUE(dict->Contains(key));
    auto it = dict->find(key);
    ASSERT_TRUE(it != dict->end());
    ASSERT_TRUE(it->first.str() == std::to_string(i));
    ASSERT_TRUE(it->second.String().str() == std::to_string(i));
  }
  for (int i = 1000; i < 1020; i++) {
    auto key = String(std::to_string(i));
    ASSERT_FALSE(dict->Contains(key));
    auto it = dict->find(key);
    ASSERT_TRUE(it == dict->end());
  }

  {
    int count = 0;
    auto keys_checker = keys;
    ASSERT_TRUE(keys_checker.size() == lepus::Dictionary::kSmallMapMaximumSize);
    for (const auto& p : *dict) {
      count++;
      ASSERT_TRUE(p.first.str() == p.second.String().str());
      keys_checker.erase(p.first.str());
    }
    ASSERT_TRUE(count == lepus::Dictionary::kSmallMapMaximumSize);
    ASSERT_TRUE(keys_checker.empty());
  }

  constexpr size_t ExtraCount = 50;

  for (int i = (int)lepus::Dictionary::kSmallMapMaximumSize;
       i < (int)(lepus::Dictionary::kSmallMapMaximumSize + ExtraCount); i++) {
    dict->SetValue(String(std::to_string(i)), std::to_string(i));
    keys.insert(std::to_string(i));
  }
  ASSERT_TRUE(dict->size() ==
              lepus::Dictionary::kSmallMapMaximumSize + ExtraCount);
  ASSERT_FALSE(dict->using_small_map());

  for (int i = 0;
       i < (int)(lepus::Dictionary::kSmallMapMaximumSize + ExtraCount); i++) {
    auto key = String(std::to_string(i));
    ASSERT_TRUE(dict->Contains(key));
    auto it = dict->find(key);
    ASSERT_TRUE(it != dict->end());
    ASSERT_TRUE(it->first.str() == std::to_string(i));
    ASSERT_TRUE(it->second.String().str() == std::to_string(i));
  }
  for (int i = 1000; i < 1020; i++) {
    auto key = String(std::to_string(i));
    ASSERT_FALSE(dict->Contains(key));
    auto it = dict->find(key);
    ASSERT_TRUE(it == dict->end());
  }

  {
    int count = 0;
    auto keys_checker = keys;
    ASSERT_TRUE(keys_checker.size() ==
                lepus::Dictionary::kSmallMapMaximumSize + ExtraCount);
    for (const auto& p : *dict) {
      count++;
      ASSERT_TRUE(p.first.str() == p.second.String().str());
      keys_checker.erase(p.first.str());
    }
    ASSERT_TRUE(count == lepus::Dictionary::kSmallMapMaximumSize + ExtraCount);
    ASSERT_TRUE(keys_checker.empty());
  }

  auto dict2 = lepus::Dictionary::Create();
  for (int i = 0;
       i < (int)(lepus::Dictionary::kSmallMapMaximumSize + ExtraCount); i++) {
    if (i % 2 == 0) {
      ASSERT_TRUE(dict->EraseKey(String(std::to_string(i))) == 1);
    } else {
      dict2->SetValue(String(std::to_string(i)), std::to_string(i));
    }
  }
  ASSERT_TRUE(*dict == *dict2);

  auto dict3 = lepus::Dictionary::Create();
  auto dict4 = lepus::Dictionary::Create();
  auto dict5 = lepus::Dictionary::Create();
  for (int i = 0; i < (int)lepus::Dictionary::kSmallMapMaximumSize; i++) {
    dict3->SetValue(String(std::to_string(i)), std::to_string(i));
  }
  for (int i = (int)(lepus::Dictionary::kSmallMapMaximumSize - 1); i >= 0;
       i--) {
    dict4->SetValue(String(std::to_string(i)), std::to_string(i));
    dict5->SetValue(String(std::to_string(i)), std::to_string(i));
  }
  ASSERT_TRUE(dict3->using_small_map());
  ASSERT_TRUE(dict4->using_small_map());
  ASSERT_TRUE(dict5->using_small_map());
  dict5->SetValue(
      String(std::to_string(lepus::Dictionary::kSmallMapMaximumSize)),
      std::to_string(lepus::Dictionary::kSmallMapMaximumSize));
  ASSERT_FALSE(dict5->using_small_map());  // dict5 transferred
  ASSERT_TRUE(*dict3 == *dict4);
  ASSERT_FALSE(*dict3 == *dict5);
  dict5->EraseKey(
      String(std::to_string(lepus::Dictionary::kSmallMapMaximumSize)));
  ASSERT_TRUE(*dict3 == *dict5);
  dict4->SetValue(String(std::to_string(1)), "1");
  ASSERT_TRUE(dict4->using_small_map());
  ASSERT_TRUE(*dict3 == *dict4);
  dict4->SetValue(String(std::to_string(1)), "1111111");
  ASSERT_TRUE(dict4->using_small_map());
  ASSERT_FALSE(*dict3 == *dict4);

  // Overwrite values
  auto dict6 = lepus::Dictionary::Create();
  auto dict7 = lepus::Dictionary::Create();
  for (int i = 0; i < (int)(lepus::Dictionary::kSmallMapMaximumSize - 1); i++) {
    // Use `lepus::Dictionary::kSmallMapMaximumSize - 1` so that later emplace
    // on existing keys will not trigger transfer.
    dict6->SetValue(String(std::to_string(i)), std::to_string(i));
    dict7->SetValue(String(std::to_string(i)), (i % 2 == 0)
                                                   ? std::to_string(i) + "_even"
                                                   : std::to_string(i));
  }
  ASSERT_TRUE(dict6->using_small_map());
  for (int i = 0; i < (int)(lepus::Dictionary::kSmallMapMaximumSize - 1); i++) {
    if (i % 2 == 0) {
      dict6->SetValue(String(std::to_string(i)), std::to_string(i) + "_even");
    }
  }
  ASSERT_TRUE(dict6->using_small_map());
  ASSERT_TRUE(*dict6 == *dict7);

  // SetValue using Value instance of self, no change made to dictionary.
  for (int i = 0; i < (int)(lepus::Dictionary::kSmallMapMaximumSize - 1); i++) {
    auto key = String(std::to_string(i));
    const auto& value = *(dict6->GetValue(key));
    dict6->SetValue(key, value);
  }
  ASSERT_TRUE(dict6->using_small_map());
  ASSERT_TRUE(*dict6 == *dict7);

  auto dict8 = lepus::Dictionary::Create(
      {{"a", lepus::Value("1")}, {"b", lepus::Value("2")}});
  ASSERT_TRUE(dict8->using_small_map());
  ASSERT_TRUE(dict8->size() == 2);
  ASSERT_TRUE(dict8->Contains("a"));
  ASSERT_TRUE(dict8->Contains("b"));
  ASSERT_FALSE(dict8->Contains("c"));
}

static inline void TestLynxValueIteratorCallback(lynx_api_env env,
                                                 lynx_value key,
                                                 lynx_value value, void* pfunc,
                                                 void* raw_data) {
  reinterpret_cast<lepus::ExtendedValueIteratorCallback*>(pfunc)->operator()(
      env, key, value);
}

static inline void TestIterateLynxValue(
    const lynx_value& val, lepus::ExtendedValueIteratorCallback* pfunc) {
  lynx_value_iterate_value(nullptr, val, TestLynxValueIteratorCallback,
                           reinterpret_cast<void*>(pfunc), nullptr);
}

TEST_F(BaseValueTest, LynxValueAPI) {
  lynx_api_env env = nullptr;
  lynx_value string_value;
  lynx_value_create_string_utf8(env, "hello lynx_value", 16, &string_value);
  lynx_value_type t1;
  lynx_value_typeof(env, string_value, &t1);
  ASSERT_TRUE(t1 == lynx_value_string);
  size_t length = 0;
  lynx_value_get_string_utf8(env, string_value, nullptr, 0, &length);
  ASSERT_TRUE(length == 16);
  std::string str;
  str.resize(length);
  lynx_value_get_string_utf8(env, string_value, &str[0], length + 1, &length);
  ASSERT_TRUE(str == "hello lynx_value");
  auto* string_ptr =
      reinterpret_cast<lynx::base::RefCountedStringImpl*>(string_value.val_ptr);
  ASSERT_TRUE(string_ptr->HasOneRef());
  lynx_value_remove_reference(env, string_value, nullptr);

  lynx_value map_value;
  lynx_value_create_map(env, &map_value);
  lynx_value_type t2;
  lynx_value_typeof(env, map_value, &t2);
  ASSERT_TRUE(t2 == lynx_value_map);
  lynx_value v1;
  lynx_value_create_int32(env, 10, &v1);
  lynx_value v2;
  lynx_value_create_double(env, 3.14f, &v2);
  lynx_value v3;
  lynx_value_create_string_utf8(env, "string", 6, &v3);
  auto* v3_ptr =
      reinterpret_cast<lynx::base::RefCountedStringImpl*>(v3.val_ptr);
  ASSERT_TRUE(v3_ptr->HasOneRef());
  lynx_value_set_named_property(env, map_value, "v1", v1);
  lynx_value_set_named_property(env, map_value, "v2", v2);
  lynx_value_set_named_property(env, map_value, "v3", v3);
  lynx_value_set_named_property(env, map_value, "v3", v3);
  lynx_api_status status = lynx_value_set_named_property(env, v3, "v1", v2);
  ASSERT_TRUE(status != lynx_api_ok);
  ASSERT_FALSE(v3_ptr->HasOneRef());
  lynx_value v1_ret;
  lynx_value_get_named_property(env, map_value, "v1", &v1_ret);
  ASSERT_TRUE(v1_ret.type == lynx_value_int32);
  ASSERT_TRUE(v1_ret.val_int32 == 10);
  int32_t int32_ret;
  lynx_value_get_int32(env, v1_ret, &int32_ret);
  ASSERT_TRUE(int32_ret == 10);
  lynx_value v2_ret;
  lynx_value_get_named_property(env, map_value, "v2", &v2_ret);
  ASSERT_TRUE(v2_ret.type == lynx_value_double);
  ASSERT_TRUE(v2_ret.val_double == 3.14f);
  double d_ret;
  lynx_value_get_double(env, v2_ret, &d_ret);
  ASSERT_TRUE(d_ret == 3.14f);
  lynx_value v3_ret;
  lynx_value_get_named_property(env, map_value, "v3", &v3_ret);
  lynx_value_remove_reference(env, v3, nullptr);
  lynx_value_remove_reference(env, v3_ret, nullptr);
  int a = 0, b = 0;
  lepus::ExtendedValueIteratorCallback callback_wrap =
      [&a, &b](lynx_api_env env, const lynx_value& key,
               const lynx_value& value) {
        auto* k_ptr =
            reinterpret_cast<lynx::base::RefCountedStringImpl*>(key.val_ptr);
        auto str = k_ptr->str();
        if (str == "v1" || str == "v2" || str == "v3") {
          a++;
        }
        if (value.type == lynx_value_int32 || value.type == lynx_value_double ||
            value.type == lynx_value_string) {
          b++;
        }
        lynx_value_remove_reference(env, value, nullptr);
      };
  TestIterateLynxValue(map_value, &callback_wrap);
  EXPECT_EQ(a, 3);
  EXPECT_EQ(b, 3);
  ASSERT_TRUE(v3_ptr->HasOneRef());
  bool has_property;
  lynx_value_has_property(env, map_value, "v1", &has_property);
  ASSERT_TRUE(has_property);
  lynx_value_remove_reference(env, map_value, nullptr);

  lynx_value array_value;
  lynx_value_create_array(env, &array_value);
  lynx_value_type t3;
  lynx_value_typeof(env, array_value, &t3);
  ASSERT_TRUE(t3 == lynx_value_array);
  lynx_value v4;
  lynx_value_create_bool(env, true, &v4);
  lynx_value v5;
  lynx_value_create_int64(env, 100, &v5);
  lynx_value v6;
  lynx_value_create_string_utf8(env, "string", 6, &v6);
  auto* v6_ptr =
      reinterpret_cast<lynx::base::RefCountedStringImpl*>(v6.val_ptr);
  ASSERT_TRUE(v6_ptr->HasOneRef());
  lynx_value_set_element(env, array_value, 0, v4);
  lynx_value_set_element(env, array_value, 1, v5);
  lynx_value_set_element(env, array_value, 2, v6);
  lynx_value_set_element(env, array_value, 2, v6);
  status = lynx_value_set_element(env, v6, 2, v5);
  ASSERT_TRUE(status != lynx_api_ok);
  ASSERT_FALSE(v6_ptr->HasOneRef());
  lynx_value v4_ret;
  lynx_value_get_element(env, array_value, 0, &v4_ret);
  ASSERT_TRUE(v4_ret.type == lynx_value_bool);
  ASSERT_TRUE(v4_ret.val_bool);
  bool b_ret;
  lynx_value_get_bool(env, v4_ret, &b_ret);
  ASSERT_TRUE(b_ret);
  lynx_value v5_ret;
  lynx_value_get_element(env, array_value, 1, &v5_ret);
  ASSERT_TRUE(v5_ret.type == lynx_value_int64);
  ASSERT_TRUE(v5_ret.val_int64 = 100);
  int64_t int64_ret;
  lynx_value_get_int64(env, v5_ret, &int64_ret);
  ASSERT_TRUE(int64_ret == 100);
  lynx_value v6_ret;
  lynx_value_get_element(env, array_value, 2, &v6_ret);
  lynx_value_remove_reference(env, v6, nullptr);
  lynx_value_remove_reference(env, v6_ret, nullptr);
  ASSERT_TRUE(v6_ptr->HasOneRef());
  lynx_value_remove_reference(env, array_value, nullptr);
  lynx_value v7;
  lynx_value_create_uint32(env, 1001, &v7);
  ASSERT_TRUE(v7.type == lynx_value_uint32);
  ASSERT_TRUE(v7.val_uint32 == 1001);
  lynx_value v8;
  lynx_value_create_uint64(env, 10001, &v8);
  ASSERT_TRUE(v8.type == lynx_value_uint64);
  ASSERT_TRUE(v8.val_uint64 == 10001);
}

}  // namespace base
}  // namespace lynx
