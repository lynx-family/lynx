// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/lynx_value_lepusng.h"

#include <cstdint>

#include "base/include/value/base_string.h"
#include "core/runtime/vm/lepus/lepus_value.h"
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
        env_(new lynx_api_env__()),
        cell_(new ContextCell(nullptr, ctx_, rt_)) {
    LEPUS_SetContextOpaque(ctx_, cell_);
    LEPUSLepusRefCallbacks callbacks = Context::GetLepusRefCall();
    RegisterLepusRefCallbacks(rt_, &callbacks);
    lynx_attach_value_lepusng(env_, ctx_);
  }

  ~LynxValueLepusNGTest() {
    delete cell_;
    lynx_detach_value_lepusng(env_);
    delete env_;
    env_ = nullptr;
    LEPUS_FreeContext(ctx_);
    LEPUS_FreeRuntime(rt_);
  }

  lynx_value ToLynxValue(const LEPUSValue& val) {
    return {
        .val_ptr = reinterpret_cast<lynx_value_ptr>(LEPUS_VALUE_GET_INT64(val)),
        .type = lynx_value_extended,
        .tag = LEPUS_VALUE_GET_TAG(val)};
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
  status = env_->lynx_value_typeof(env_, l_val_null, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_null);
  status = env_->lynx_value_typeof(env_, l_val_undefined, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_undefined);
}

TEST_F(LynxValueLepusNGTest, LynxValueBool) {
  lynx_value_type type;
  lynx_api_status status;

  LEPUSValue val_bool = LEPUS_NewBool(ctx_, false);
  lynx_value l_val_bool = ToLynxValue(val_bool);
  status = env_->lynx_value_typeof(env_, l_val_bool, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_bool);
  bool result;
  status = env_->lynx_value_get_bool(env_, l_val_bool, &result);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(!result);
  LEPUS_FreeValue(ctx_, val_bool);
}

TEST_F(LynxValueLepusNGTest, LynxValueNumber) {
  lynx_value_type type;
  lynx_api_status status;

  LEPUSValue val_int32 = LEPUS_NewInt32(ctx_, 1024);
  lynx_value l_val_int32 = ToLynxValue(val_int32);
  status = env_->lynx_value_typeof(env_, l_val_int32, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_int32);
  int32_t ret_int32;
  status = env_->lynx_value_get_int32(env_, l_val_int32, &ret_int32);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(ret_int32 == 1024);

  int64_t num = (int64_t)INT32_MAX + 1;
  LEPUSValue val_int64 = LEPUS_NewInt64(ctx_, num);
  lynx_value l_val_int64 = ToLynxValue(val_int64);
  status = env_->lynx_value_typeof(env_, l_val_int64, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_int64);
  int64_t ret_int64;
  status = env_->lynx_value_get_int64(env_, l_val_int64, &ret_int64);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(ret_int64 == (int64_t)INT32_MAX + 1);

  LEPUSValue val_double = LEPUS_NewFloat64(ctx_, 3.14f);
  lynx_value l_val_double = ToLynxValue(val_double);
  status = env_->lynx_value_typeof(env_, l_val_double, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_double);
  double ret_double;
  status = env_->lynx_value_get_double(env_, l_val_double, &ret_double);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(ret_double - 3.14 < 0.001);

  double ret_number1;
  status = env_->lynx_value_get_number(env_, l_val_double, &ret_number1);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(ret_number1 - 3.14 < 0.001);

  double ret_number2;
  status = env_->lynx_value_get_number(env_, l_val_int32, &ret_number2);
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
  status = env_->lynx_value_typeof(env_, l_val_str, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_string);

  bool has_str_ref;
  status = env_->lynx_value_has_string_ref(env_, l_val_str, &has_str_ref);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(has_str_ref);
  void* str_ref;
  status = env_->lynx_value_get_string_ref(env_, l_val_str, &str_ref);
  ASSERT_TRUE(status == lynx_api_ok);
  auto* base_str = reinterpret_cast<base::RefCountedStringImpl*>(str_ref);
  ASSERT_TRUE(base_str->str() == "test");

  size_t length;
  status =
      env_->lynx_value_get_string_utf8(env_, l_val_str, nullptr, 0, &length);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(length == 4);
  std::string value;
  value.reserve(length + 1);
  value.resize(length);
  status = env_->lynx_value_get_string_utf8(env_, l_val_str, &value[0],
                                            value.capacity(), nullptr);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(value == "test");

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
  status = env_->lynx_value_typeof(env_, l_val_arr, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_object);

  bool is_array;
  status = env_->lynx_value_is_array(env_, l_val_arr, &is_array);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(is_array);

  uint32_t length1;
  status = env_->lynx_value_get_array_length(env_, l_val_arr, &length1);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(length1 == 10);
  uint32_t length2;
  status = env_->lynx_value_get_length(env_, l_val_arr, &length2);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(length2 == 10);

  lynx_value element5;
  status = env_->lynx_value_get_element(env_, l_val_arr, 5, &element5);
  ASSERT_TRUE(status == lynx_api_ok);
  int32_t ret_int32;
  status = env_->lynx_value_get_int32(env_, element5, &ret_int32);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(ret_int32 == 5);

  lynx_value l_val_int32 = ToLynxValue(LEPUS_NewInt32(ctx_, 1024));
  status = env_->lynx_value_set_element(env_, l_val_arr, 10, l_val_int32);
  ASSERT_TRUE(status == lynx_api_ok);
  bool has_element;
  status = env_->lynx_value_has_element(env_, l_val_arr, 10, &has_element);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(has_element);

  bool delete_ret;
  status = env_->lynx_value_delete_element(env_, l_val_arr, 10, &delete_ret);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(delete_ret);

  lynx_value copied_array;
  status = env_->lynx_value_deep_copy_value(env_, l_val_arr, &copied_array);
  ASSERT_TRUE(status == lynx_api_ok);
  bool is_equal;
  status = env_->lynx_value_equals(env_, l_val_arr, copied_array, &is_equal);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(is_equal);

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
  status = env_->lynx_value_set_element(env_, l_val_arr, 0, l_val_str);
  ASSERT_TRUE(status == lynx_api_ok);
  p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
  ASSERT_TRUE(p->ref_count == 2);
  lynx_value result;
  status = env_->lynx_value_get_element(env_, l_val_arr, 0, &result);
  ASSERT_TRUE(status == lynx_api_ok);
  p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
  ASSERT_TRUE(p->ref_count == 3);

  {
    auto lepus_value = lepus::Value(ctx_, val_str);
    p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
    ASSERT_TRUE(p->ref_count == 4);
  }
  p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
  ASSERT_TRUE(p->ref_count == 3);
  {
    auto lepus_arr = lepus::Value(ctx_, val_arr);
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
  status = env_->lynx_value_typeof(env_, l_val_map, &type);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(type == lynx_value_object);

  uint32_t length;
  status = env_->lynx_value_get_length(env_, l_val_map, &length);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(length == 1);

  bool has_property;
  status = env_->lynx_value_has_named_property(env_, l_val_map, "key1",
                                               &has_property);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(has_property);

  lynx_value property;
  status =
      env_->lynx_value_get_named_property(env_, l_val_map, "key1", &property);
  ASSERT_TRUE(status == lynx_api_ok);
  int32_t ret_int32;
  status = env_->lynx_value_get_int32(env_, property, &ret_int32);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(ret_int32 == 10);

  lynx_value l_val_int32 = ToLynxValue(LEPUS_NewInt32(ctx_, 1024));
  status =
      env_->lynx_value_set_named_property(env_, l_val_map, "key2", l_val_int32);
  ASSERT_TRUE(status == lynx_api_ok);
  uint32_t length1;
  status = env_->lynx_value_get_length(env_, l_val_map, &length1);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(length1 == 2);

  lynx_value property1;
  status =
      env_->lynx_value_get_named_property(env_, l_val_map, "key2", &property1);
  ASSERT_TRUE(status == lynx_api_ok);
  bool is_equal;
  status = env_->lynx_value_equals(env_, l_val_int32, property1, &is_equal);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(is_equal);

  status = env_->lynx_value_delete_named_property(env_, l_val_map, "key2");
  ASSERT_TRUE(status == lynx_api_ok);
  bool has_property1;
  status = env_->lynx_value_has_named_property(env_, l_val_map, "key2",
                                               &has_property1);
  ASSERT_TRUE(status == lynx_api_ok);
  ASSERT_TRUE(!has_property1);

  LEPUS_FreeValue(ctx_, obj);
}

TEST_F(LynxValueLepusNGTest, LynxValueReference) {
  lynx_api_status status;
  LEPUSValue val_str = LEPUS_NewStringLen(ctx_, "test", 4);
  LEPUSRefCountHeader* p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
  ASSERT_TRUE(p->ref_count == 1);
  lynx_value l_val_str = ToLynxValue(val_str);
  lynx_value_ref ref;
  status = env_->lynx_value_create_reference(env_, l_val_str, 1, &ref);
  ASSERT_TRUE(status == lynx_api_ok);
  p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
  ASSERT_TRUE(p->ref_count == 2);
  status = env_->lynx_value_delete_reference(env_, ref);
  ASSERT_TRUE(status == lynx_api_ok);
  p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
  ASSERT_TRUE(p->ref_count == 1);
  lynx_value_ref move_ref;
  status = env_->lynx_value_move_reference(env_, l_val_str, ref, &move_ref);
  ASSERT_TRUE(status == lynx_api_ok);
  p = (LEPUSRefCountHeader*)LEPUS_VALUE_GET_PTR(val_str);
  ASSERT_TRUE(p->ref_count == 1);
}

}  // namespace test
}  // namespace lepus
}  // namespace lynx
