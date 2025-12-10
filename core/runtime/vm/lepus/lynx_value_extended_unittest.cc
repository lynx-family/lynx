// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "base/include/value/lynx_value_extended.h"

#include <cstdint>

#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/value/base_value.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace lepus {
namespace test {
class LynxValueLepusNGTest : public ::testing::Test {
 protected:
  LynxValueLepusNGTest()
      : rt_(LEPUS_NewRuntime()),
        ctx_(LEPUS_NewContext(rt_)),
        env_(lynx_value_api_new_env(ctx_)),
        cell_(new ContextCell(nullptr, ctx_, rt_)) {
    LEPUS_SetContextOpaque(ctx_, cell_);
    LEPUSLepusRefCallbacks callbacks = QuickContext::GetLepusRefCall();
    RegisterLepusRefCallbacks(rt_, &callbacks);
  }

  ~LynxValueLepusNGTest() {
    delete cell_;
    lynx_value_api_delete_env(env_);
    LEPUS_FreeContext(ctx_);
    LEPUS_FreeRuntime(rt_);
  }

  lynx_value ToLynxValue(const LEPUSValue& val) {
    int64_t val_tag = LEPUS_VALUE_GET_TAG(val);
    int32_t tag =
        (static_cast<int32_t>(
             lynx::lepus::LEPUSValueHelper::LEPUSValueTagToLynxValueType(
                 val_tag))
         << 16) |
        (val_tag & 0xff);
    return MAKE_LYNX_VALUE(val, tag);
  }

  LEPUSRuntime* rt_;
  LEPUSContext* ctx_;
  lynx_api_env env_;
  ContextCell* cell_;
};

TEST_F(LynxValueLepusNGTest, LynxValueNull) {
  lynx_value_type type;
  lynx_api_status status;

  LEPUSValue val_null{LEPUS_NULL};
  LEPUSValue val_undefined{LEPUS_UNDEFINED};

  lynx_value l_val_null = ToLynxValue(val_null);
  lynx_value l_val_undefined = ToLynxValue(val_undefined);
  status = lynx_value_typeof_ext(env_, l_val_null, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_null);
  status = lynx_value_typeof_ext(env_, l_val_undefined, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_undefined);
}

TEST_F(LynxValueLepusNGTest, LynxValueBool) {
  lynx_value_type type;
  lynx_api_status status;

  LEPUSValue val_bool = LEPUS_NewBool(ctx_, false);
  lynx_value l_val_bool = ToLynxValue(val_bool);
  status = lynx_value_typeof_ext(env_, l_val_bool, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_bool);
  bool result;
  status = lynx_value_get_bool_ext(env_, l_val_bool, &result);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(!result);
  LEPUS_FreeValue(ctx_, val_bool);
}

TEST_F(LynxValueLepusNGTest, LynxValueNumber) {
  lynx_value_type type;
  lynx_api_status status;

  LEPUSValue val_int32 = LEPUS_NewInt32(ctx_, 1024);
  lynx_value l_val_int32 = ToLynxValue(val_int32);
  status = lynx_value_typeof_ext(env_, l_val_int32, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_int32);

  int64_t num = (int64_t)INT32_MAX + 1;
  LEPUSValue val_int64 = LEPUS_NewInt64(ctx_, num);
  lynx_value l_val_int64 = ToLynxValue(val_int64);
  status = lynx_value_typeof_ext(env_, l_val_int64, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_int64);

  LEPUSValue val_double = LEPUS_NewFloat64(ctx_, 3.14f);
  lynx_value l_val_double = ToLynxValue(val_double);
  status = lynx_value_typeof_ext(env_, l_val_double, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_double);

  double ret_number1;
  status = lynx_value_get_number_ext(env_, l_val_double, &ret_number1);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(ret_number1 - 3.14 < 0.001);

  double ret_number2;
  status = lynx_value_get_number_ext(env_, l_val_int32, &ret_number2);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(ret_number2 - 1024 < 0.001);

  LEPUS_FreeValue(ctx_, val_int32);
  LEPUS_FreeValue(ctx_, val_int64);
  LEPUS_FreeValue(ctx_, val_double);
}

TEST_F(LynxValueLepusNGTest, LynxValueString) {
  lynx_value_type type;
  lynx_api_status status;
  LEPUSValue val_str = LEPUS_NewStringLen(ctx_, "test", 4);
  lynx_value l_val_str = ToLynxValue(val_str);
  status = lynx_value_typeof_ext(env_, l_val_str, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_string);
  void* str_ref;
  status = lynx_value_get_string_ref_ext(env_, l_val_str, &str_ref);
  ASSERT_TRUE(status == lynx_api_ok);
  auto* base_str = reinterpret_cast<base::RefCountedStringImpl*>(str_ref);
  ASSERT_TRUE(base_str->str() == "test");

  std::string str;
  status = lynx_value_to_string_utf8_ext(env_, l_val_str, &str);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(str == "test");

  LEPUS_FreeValue(ctx_, val_str);
}

TEST_F(LynxValueLepusNGTest, LynxValueArray) {
  lynx_value_type type;
  lynx_api_status status;

  std::vector<LEPUSValue> values;
  values.reserve(10);
  for (int32_t i = 0; i < 10; i++) {
    values.emplace_back(LEPUS_NewInt32(ctx_, i));
  }
  LEPUSValue arr = LEPUS_NewArrayWithValue(ctx_, 10, values.data());
  lynx_value l_val_arr = ToLynxValue(arr);
  status = lynx_value_typeof_ext(env_, l_val_arr, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_array);

  bool is_array;
  status = lynx_value_is_array_ext(env_, l_val_arr, &is_array);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(is_array);

  uint32_t length1;
  status = lynx_value_get_length_ext(env_, l_val_arr, &length1);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(length1 == 10);
  uint32_t length2;
  status = lynx_value_get_length_ext(env_, l_val_arr, &length2);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(length2 == 10);

  lynx_value element5;
  status = lynx_value_get_element_ext(env_, l_val_arr, 5, &element5);
  ASSERT_TRUE(status == lynx_api_ok);

  lynx_value l_val_int32 = ToLynxValue(LEPUS_NewInt32(ctx_, 1024));
  status = lynx_value_set_element_ext(env_, l_val_arr, 10, l_val_int32);
  ASSERT_TRUE(status == lynx_api_ok);

  LEPUS_FreeValue(ctx_, arr);
}

TEST_F(LynxValueLepusNGTest, LynxValueMemory) {
  lynx_api_status status;
  LEPUSValue val_str = LEPUS_NewStringLen(ctx_, "test", 4);
  lynx_value l_val_str = ToLynxValue(val_str);
  LEPUSValue val_arr = LEPUS_NewArray(ctx_);
  lynx_value l_val_arr = ToLynxValue(val_arr);
  LEPUSRefCountHeader* p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
  ASSERT_TRUE(p->ref_count == 1);
  status = lynx_value_set_element_ext(env_, l_val_arr, 0, l_val_str);
  ASSERT_TRUE(status == lynx_api_ok);
  p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
  ASSERT_TRUE(p->ref_count == 2);
  lynx_value result;
  status = lynx_value_get_element_ext(env_, l_val_arr, 0, &result);
  ASSERT_TRUE(status == lynx_api_ok);
  p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
  ASSERT_TRUE(p->ref_count == 3);

  {
    auto lepus_value = MK_JS_LEPUS_VALUE(ctx_, val_str);
    p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
    ASSERT_TRUE(p->ref_count == 4);
  }
  p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
  ASSERT_TRUE(p->ref_count == 3);
  {
    auto lepus_arr = MK_JS_LEPUS_VALUE(ctx_, val_arr);
    auto ret = lepus_arr.GetProperty(0);
    p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
    ASSERT_TRUE(p->ref_count == 4);
  }
  p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
  ASSERT_TRUE(p->ref_count == 3);

  LEPUS_FreeValue(ctx_, val_str);
  LEPUS_FreeValue(ctx_, val_arr);
}

TEST_F(LynxValueLepusNGTest, LynxValueMap) {
  lynx_value_type type;
  lynx_api_status status;

  LEPUSValue obj = LEPUS_NewObject(ctx_);
  LEPUS_SetPropertyStr(ctx_, obj, "key1", LEPUS_NewInt32(ctx_, 10));
  lynx_value l_val_map = ToLynxValue(obj);
  status = lynx_value_typeof_ext(env_, l_val_map, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_map);

  uint32_t length;
  status = lynx_value_get_length_ext(env_, l_val_map, &length);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(length == 1);

  bool has_property;
  status =
      lynx_value_has_named_property_ext(env_, l_val_map, "key1", &has_property);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(has_property);

  lynx_value property;
  status =
      lynx_value_get_named_property_ext(env_, l_val_map, "key1", &property);
  ASSERT_TRUE(status == lynx_api_ok);

  lynx_value l_val_int32 = ToLynxValue(LEPUS_NewInt32(ctx_, 1024));
  status =
      lynx_value_set_named_property_ext(env_, l_val_map, "key2", l_val_int32);
  ASSERT_TRUE(status == lynx_api_ok);
  uint32_t length1;
  status = lynx_value_get_length_ext(env_, l_val_map, &length1);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(length1 == 2);

  lynx_value property1;
  status =
      lynx_value_get_named_property_ext(env_, l_val_map, "key2", &property1);
  ASSERT_TRUE(status == lynx_api_ok);

  bool has_property1;
  status = lynx_value_has_named_property_ext(env_, l_val_map, "key2",
                                             &has_property1);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(has_property1);

  LEPUS_FreeValue(ctx_, obj);
}

TEST_F(LynxValueLepusNGTest, LynxValueRefCounted) {
  lynx_api_status status;
  auto array = lepus::CArray::Create();
  auto* array_ptr = array.get();
  array_ptr->AddRef();
  LEPUSValue val = LEPUS_NewLepusWrap(ctx_, array_ptr, lepus::Value_Array);
  lynx_value l_val_arr = ToLynxValue(val);

  lynx_value_type type;
  status = lynx_value_typeof_ext(env_, l_val_arr, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_array);

  std::string str;
  status = lynx_value_to_string_utf8_ext(env_, l_val_arr, &str);
  ASSERT_TRUE(status == lynx_api_ok);
  LOGI("str: " << str);

  bool is_arr;
  status = lynx_value_is_array_ext(env_, l_val_arr, &is_arr);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(is_arr);

  LEPUS_FreeValue(ctx_, val);
}

}  // namespace test
}  // namespace lepus
}  // namespace lynx
