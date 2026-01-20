// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/lepus/table_api.h"

#include <utility>

#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {

static RestrictedValue Freeze(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  RestrictedValue object(context->GetParam(0)->Table());
  RestrictedValue result(Dictionary::Create());
  auto object_table = object.Table();
  auto result_table = result.Table();
  result_table->reserve(object_table->size());
  for (auto& iter : *object_table) {
    Dictionary::Unsafe::SetValueUniqueKey(*result_table, iter.first,
                                          iter.second);
  }
  return result;
}

static RestrictedValue Keys(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  auto* param = context->GetParam(0);
  RestrictedValue result(CArray::Create());
  auto result_array = result.Array();
  if (param->IsArray()) {
    size_t array_size =
        RestrictedValue::Unsafe::TypeSure::GetArray(*param)->size();
    result_array->reserve(array_size);
    for (size_t i = 0; i < array_size; i++) {
      result_array->emplace_back(std::to_string(i));
    }
  } else if (param->IsTable()) {
    auto param_table = RestrictedValue::Unsafe::TypeSure::GetTable(*param);
    result_array->reserve(param_table->size());
    param_table->for_each(
        [&](const auto& key, const auto&) { result_array->emplace_back(key); });
  }
  return result;
}

static RestrictedValue Assign(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count >= 1);
  auto* target = context->GetParam(0);
  if (target->IsTable()) {
    auto target_table = target->Table();
    for (int32_t i = 1; i < params_count; i++) {
      auto* source = context->GetParam(i);
      if (source->IsTable()) {
        auto source_table =
            RestrictedValue::Unsafe::TypeSure::GetTable(*source);
        for (const auto& iter : *source_table) {
          target_table->SetValue(iter.first, iter.second);
        }
      }
    }
  } else if (target->IsArray()) {
    auto target_array = RestrictedValue::Unsafe::TypeSure::GetArray(*target);
    for (int32_t i = 1; i < params_count; i++) {
      auto* source = context->GetParam(i);
      int32_t index = 0;
      if (source->IsArray()) {
        auto source_array =
            RestrictedValue::Unsafe::TypeSure::GetArray(*source);
        size_t array_size = source_array->size();
        for (size_t j = 0; j < array_size; j++) {
          target_array->set(index++, source_array->get(j));
        }
      }
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
