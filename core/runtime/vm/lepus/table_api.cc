// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/table_api.h"

#include <utility>

#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace lepus {

static Value Freeze(VMContext* context, Value*, int) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  Value object = Value(context->GetParam(0)->Table());
  Value result = Value(Dictionary::Create());
  auto object_table = object.Table();
  auto result_table = result.Table();
  result_table->reserve(object_table->size());
  for (auto iter = object_table->begin(); iter != object_table->end(); iter++) {
    Dictionary::Unsafe::SetValueUniqueKey(*result_table, iter->first,
                                          iter->second);
  }
  return result;
}

static Value Keys(VMContext* context, Value*, int) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  Value* param = context->GetParam(0);
  Value result = Value(CArray::Create());
  auto result_array = result.Array();
  if (param->IsArray()) {
    size_t array_size = param->Array()->size();
    result_array->reserve(array_size);
    for (size_t i = 0; i < array_size; i++) {
      result_array->emplace_back(std::to_string(i));
    }
  } else if (param->IsTable()) {
    auto param_table = param->Table();
    result_array->reserve(param_table->size());
    param_table->for_each(
        [&](const auto& key, const auto&) { result_array->emplace_back(key); });
  }
  return result;
}

static Value Assign(VMContext* context, Value*, int) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count >= 1);
  Value* target = context->GetParam(0);
  switch (target->Type()) {
    case Value_Table: {
      auto target_table = target->Table();
      for (int32_t i = 1; i < params_count; i++) {
        Value* source = context->GetParam(i);
        if (source->IsTable()) {
          for (const auto& iter : *(source->Table())) {
            target_table->SetValue(iter.first, iter.second);
          }
        }
      }
      break;
    }
    case Value_Array: {
      auto target_array = target->Array();
      for (int32_t i = 1; i < params_count; i++) {
        Value* source = context->GetParam(i);
        int32_t index = 0;
        if (source->IsArray()) {
          auto source_array = source->Array();
          size_t array_size = source_array->size();
          for (size_t j = 0; j < array_size; j++) {
            target_array->set(index++, source_array->get(j));
          }
        }
      }
      break;
    }
    default: {
      break;
    }
  }
  return *target;
}

void RegisterTableAPI(Context* ctx) {
  static BuiltinFunctionTable apis(BuiltinFunctionTable::Object,
                                   {
                                       {"assign", &Assign},
                                       {"freeze", &Freeze},
                                       {"keys", &Keys},
                                   });
  RegisterBuiltinFunctionTable(ctx, "Object", &apis);
}
}  // namespace lepus
}  // namespace lynx
