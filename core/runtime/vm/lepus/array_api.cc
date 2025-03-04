// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/array_api.h"

#include <string>
#include <utility>

#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_date.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace lepus {

static Value Push(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count >= 1);
  Value* this_obj = context->GetParam(params_count - 1);
  DCHECK(this_obj->IsArray());

  auto this_array = this_obj->Array();
  if (params_count > 8) {
    // reserve() will reallocate exactly the required size.
    // Normally if code is pushing one by one, reserve() will cause
    // more reallocations than old_capacity*2 autogrowth.
    this_array->reserve(this_array->size() + params_count - 1);
  }
  for (auto i = 0; i < params_count - 1; i++) {
    Value* val = context->GetParam(i);
    this_array->push_back(*val);
  }
  return Value(static_cast<uint64_t>(this_array->size()));
}

static Value Pop(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  Value* this_obj = context->GetParam(params_count - 1);
  DCHECK(this_obj->IsArray());
  auto this_array = this_obj->Array();
  this_array->pop_back();
  return Value(static_cast<uint64_t>(this_array->size()));
}

static Value Shift(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  Value* this_obj = context->GetParam(params_count - 1);
  DCHECK(this_obj->IsArray());
  return this_obj->Array()->get_shift();
}

static Value Map(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 2);
  Value* map_function = context->GetParam(0);
  Value* this_obj = context->GetParam(1);
  Value* index = this_obj + 1;
  Value* this_array = this_obj + 2;
  size_t length = this_obj->Array()->size();
  *this_array = *this_obj;
  Value array_temp_ = (*this_obj), ret, map_ret;
  auto ret_array = CArray::Create();
  auto array_temp_ptr = array_temp_.Array();
  ret_array->reserve(length);
  for (size_t i = 0; i < length; i++) {
    *this_obj = array_temp_ptr->get(i);
    index->SetNumber(static_cast<int64_t>(i));
    static_cast<VMContext*>(context)->CallFunction(map_function, 3, &map_ret);
    ret_array->emplace_back(std::move(map_ret));
  }
  ret.SetArray(std::move(ret_array));
  return ret;
}

static Value Filter(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 2);
  Value* filter_function = context->GetParam(0);
  Value* this_obj = context->GetParam(1);
  Value* index = this_obj + 1;
  Value* this_array = this_obj + 2;
  size_t length = this_obj->Array()->size();
  *this_array = *this_obj;
  Value array_temp_ = (*this_obj), ret, filter_ret;
  auto ret_array = CArray::Create();
  auto array_temp_ptr = array_temp_.Array();
  for (size_t i = 0; i < length; i++) {
    *this_obj = array_temp_ptr->get(i);
    index->SetNumber(static_cast<int64_t>(i));
    static_cast<VMContext*>(context)->CallFunction(filter_function, 3,
                                                   &filter_ret);
    if (filter_ret.Bool()) {
      ret_array->push_back(*this_obj);
    }
  }
  ret.SetArray(std::move(ret_array));
  return ret;
}

static Value Concat(VMContext* context) {
  auto params_count = context->GetParamsSize();
  Value* this_obj = context->GetParam(params_count - 1);
  DCHECK(this_obj->IsArray());

  auto ret_array = CArray::Create();
  auto this_array = this_obj->Array();
  ret_array->reserve(this_array->size() + params_count);
  for (size_t i = 0; i < this_array->size(); i++) {
    ret_array->push_back(this_array->get(i));
  }
  for (int i = 1; i < params_count; i++) {
    Value* param_i = context->GetParam(i - 1);
    if (param_i->IsArray()) {
      auto array_i = param_i->Array();
      ret_array->reserve(ret_array->size() + array_i->size());
      for (size_t j = 0; j < array_i->size(); j++) {
        ret_array->push_back(array_i->get(j));
      }
    } else {
      ret_array->push_back(*param_i);
    }
  }

  return Value(std::move(ret_array));
}

static std::string CastToString(const Value& v) {
  std::string result;
  switch (v.Type()) {
    case lepus::ValueType::Value_Nil:
    case lepus::ValueType::Value_Undefined:
      result = "";
      break;
    case lepus::ValueType::Value_Double:
      result = std::to_string(v.Number());
      break;
    case lepus::ValueType::Value_Int32:
      result = std::to_string(static_cast<int32_t>(v.Number()));
      break;
    case lepus::ValueType::Value_Int64:
      result = std::to_string(static_cast<int64_t>(v.Number()));
      break;
    case lepus::ValueType::Value_UInt32:
      result = std::to_string(static_cast<uint32_t>(v.Number()));
      break;
    case lepus::ValueType::Value_UInt64:
      result = std::to_string(static_cast<uint64_t>(v.Number()));
      break;
    case lepus::ValueType::Value_Bool: {
      if (v.Number()) {
        result = "true";
      } else {
        result = "false";
      }
    } break;
    case lepus::ValueType::Value_String:
      result = v.StdString();
      break;
    case lepus::ValueType::Value_Table:
      result = "[object Object]";
      break;
    case lepus::ValueType::Value_Array: {
      auto v_array = v.Array();
      for (size_t i = 0; i < v_array->size(); i++) {
        result += CastToString(v_array->get(i));
        if (i != (v_array->size() - 1)) {
          result += ',';
        }
      }
    } break;
    case lepus::ValueType::Value_RegExp: {
      auto v_reg = v.RegExp();
      result += "/";
      result += v_reg->get_pattern().str();
      result += "/";
      result += v_reg->get_flags().str();
    } break;
    case lepus::ValueType::Value_CDate: {
      std::stringstream ss;
      v.Date()->print(ss);
      result = ss.str();
      result.pop_back();
      break;
    }
    case lepus::ValueType::Value_NaN: {
      result = "NaN";
      break;
    }
    case lepus::ValueType::Value_Closure:
    case lepus::ValueType::Value_CFunction:
    case lepus::ValueType::Value_CPointer:
    case lepus::ValueType::Value_RefCounted:
    case lepus::ValueType::Value_JSObject:
      break;
    case lepus::ValueType::Value_ByteArray: {
      result = "ByteArray";
      break;
    }
    case lepus::ValueType::Value_PrimJsValue:
    case lepus::ValueType::Value_TypeCount:
      break;
  }
  return result;
}

static Value Join(VMContext* context) {
  auto params_count = context->GetParamsSize();
  Value* this_obj = context->GetParam(params_count - 1);
  DCHECK(this_obj->IsArray());

  std::string result = "";
  std::string separator = ",";
  if (params_count == 2) {
    separator = context->GetParam(0)->StdString();
  }

  auto this_obj_array = this_obj->Array();
  for (size_t i = 0; i < this_obj_array->size(); i++) {
    if (i < this_obj_array->size() - 1) {
      result += (CastToString(this_obj_array->get(i)) + separator);
    } else {
      result += (CastToString(this_obj_array->get(i)));
    }
  }
  return Value(std::move(result));
}

static Value FindIndex(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 2);
  Value* find_index_function = context->GetParam(0);
  Value* this_obj = context->GetParam(1);
  Value* index = this_obj + 1;
  Value* this_array = this_obj + 2;
  size_t length = this_obj->Array()->size();
  *this_array = *this_obj;
  Value array_temp_ = (*this_obj), find_index_ret;
  auto array_temp_ptr = array_temp_.Array();
  Value ret(-1);
  for (int i = 0; static_cast<size_t>(i) < length; i++) {
    *this_obj = array_temp_ptr->get(i);
    index->SetNumber(static_cast<int64_t>(i));
    static_cast<VMContext*>(context)->CallFunction(find_index_function, 3,
                                                   &find_index_ret);
    if ((find_index_ret.IsTrue())) {
      ret = Value(i);
      break;
    }
  }
  return ret;
}

static Value Find(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 2);
  Value* find_index_function = context->GetParam(0);
  Value* this_obj = context->GetParam(1);
  Value* index = this_obj + 1;
  Value* this_array = this_obj + 2;
  size_t length = this_obj->Array()->size();
  *this_array = *this_obj;
  Value array_temp_ = (*this_obj), ret, find_index_ret;
  auto array_temp_ptr = array_temp_.Array();
  for (size_t i = 0; i < length; i++) {
    *this_obj = array_temp_ptr->get(i);
    index->SetNumber(static_cast<int64_t>(i));
    static_cast<VMContext*>(context)->CallFunction(find_index_function, 3,
                                                   &find_index_ret);
    if ((find_index_ret.IsTrue())) {
      ret = *this_obj;
      break;
    }
  }
  return ret;
}

static Value Includes(VMContext* context) {
  auto params_count = context->GetParamsSize();
  Value* this_obj = context->GetParam(params_count - 1);
  DCHECK(this_obj->IsArray());

  if (params_count == 1) {
    return Value(false);
  }

  int64_t start_find = 0;
  auto this_array = this_obj->Array();

  if (params_count == 3) {
    int param2 = context->GetParam(1)->Number();
    if (param2 >= 0) {
      start_find = param2;
    } else {
      start_find = (this_array->size() + param2);
      start_find = start_find < 0 ? 0 : start_find;
    }
  }

  Value* param1 = context->GetParam(0);
  for (size_t i = static_cast<size_t>(start_find); i < this_array->size();
       i++) {
    if (this_array->get(i) == *param1) {
      return Value(true);
    }
  }
  return Value(false);
}

static Value ArraySlice(VMContext* context) {
  auto params_count = context->GetParamsSize();
  Value* this_val = context->GetParam(params_count - 1);
  DCHECK(this_val->IsArray());
  auto this_array = this_val->Array();
  int64_t start_index = 0;
  const auto this_array_size = this_array->size();
  size_t end_index = this_array_size;

  if (params_count != 1) {
    int param1 = context->GetParam(0)->Number();
    if (param1 >= 0) {
      start_index = param1;
    } else {
      start_index = this_array_size + param1;
      start_index = start_index < 0 ? 0 : start_index;
    }
  }

  if (params_count == 3) {
    int param2 = context->GetParam(1)->Number();
    if (param2 >= 0) {
      end_index = static_cast<size_t>(param2) > this_array_size
                      ? this_array_size
                      : param2;
    } else {
      end_index =
          (this_array_size + param2) < 0 ? 0 : (this_array_size + param2);
    }
  }

  auto ret_array = CArray::Create();
  size_t unsigned_start_index = static_cast<size_t>(start_index);
  if (unsigned_start_index < end_index) {
    ret_array->reserve(end_index - unsigned_start_index);
    for (size_t i = unsigned_start_index; i < end_index; i++) {
      ret_array->push_back(this_array->get(i));
    }
  }
  return Value(std::move(ret_array));
}

static Value ForEach(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 2);
  Value* foreach_function = context->GetParam(0);
  Value* this_obj = context->GetParam(1);
  Value* index = this_obj + 1;
  Value* this_array = this_obj + 2;
  size_t length = this_obj->Array()->size();
  *this_array = *this_obj;
  Value array_temp_ = (*this_obj), foreach_ret;
  auto array_temp_ptr = array_temp_.Array();
  for (size_t i = 0; i < length; i++) {
    *this_obj = array_temp_ptr->get(i);
    index->SetNumber(static_cast<int64_t>(i));
    static_cast<VMContext*>(context)->CallFunction(foreach_function, 3,
                                                   &foreach_ret);
  }
  return Value();
}

void RegisterArrayAPI(Context* ctx) {
  fml::RefPtr<Dictionary> table = Dictionary::Create();
  RegisterTableFunction(ctx, table, "push", &Push);
  RegisterTableFunction(ctx, table, "pop", &Pop);
  RegisterTableFunction(ctx, table, "shift", &Shift);
  RegisterTableFunction(ctx, table, "map", &Map);
  RegisterTableFunction(ctx, table, "filter", &Filter);
  RegisterTableFunction(ctx, table, "concat", &Concat);
  RegisterTableFunction(ctx, table, "join", &Join);
  RegisterTableFunction(ctx, table, "findIndex", &FindIndex);
  RegisterTableFunction(ctx, table, "find", &Find);
  RegisterTableFunction(ctx, table, "includes", &Includes);
  RegisterTableFunction(ctx, table, "slice", &ArraySlice);
  RegisterTableFunction(ctx, table, "forEach", &ForEach);
  reinterpret_cast<VMContext*>(ctx)->SetArrayPrototype(Value(std::move(table)));
}
}  // namespace lepus
}  // namespace lynx
