// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/value_utils.h"

#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace tasm {

// shadow equal check for table value
bool CheckTableValueNotEqual(const lepus::Value& target_item_value,
                             const lepus::Value& update_item_value) {
  if (update_item_value.Type() != target_item_value.Type()) {
    return true;
  }

  switch (update_item_value.Type()) {
    case lepus::Value_Table:
      return true;
    case lepus::Value_Array:
      return true;
    default:
      return update_item_value != target_item_value;
  }
}
#if ENABLE_INSPECTOR && (ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE)
bool CheckTableDeepUpdated(const lepus::Value& target,
                           const lepus::Value& update, bool first_layer) {
  auto target_type = target.Type();
  if (target_type != update.Type()) {
    return true;
  }

  if (target_type == lepus::Value_Table) {
    // component new data from setData
    auto update_table_value = update.Table();
    // component current data table
    auto target_table_value = target.Table();
    // if two tables' size are different, need update;
    if (update_table_value->size() != target_table_value->size() &&
        !first_layer) {
      return true;
    }
    // deep compare current_data_table && new_data_table top level
    // if any top level data are different, need update;
    for (auto& update_data_iterator : *update_table_value) {
      auto target_item_iterator =
          target_table_value->find(update_data_iterator.first);

      if (target_item_iterator == target_table_value->end()) {
        // target did not have this new key
        return true;
      }

      lepus::Value target_item_value = target_item_iterator->second;
      lepus::Value update_item_value = update_data_iterator.second;

      if (CheckTableDeepUpdated(target_item_value, update_item_value, false)) {
        return true;
      }
    }
    return false;
  } else {
    return target != update;
  }
}
#endif
// shadow equal for table
bool CheckTableShadowUpdated(const lepus::Value& target,
                             const lepus::Value& update) {
#if ENABLE_INSPECTOR && (ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE)
  if (lynx::tasm::LynxEnv::GetInstance().IsTableDeepCheckEnabled()) {
    return CheckTableDeepUpdated(target, update, true);
  }
#endif

  auto target_type = target.Type();
  if (target_type != update.Type()) {
    return true;
  }

  if (target_type == lepus::Value_Table) {
    // component new data from setData
    auto update_table_value = update.Table();
    // component current data table
    auto target_table_value = target.Table();
    // shadow compare current_data_table && new_data_table top level
    // if any top level data are different, need update;
    for (auto& update_data_iterator : *update_table_value) {
      auto target_item_iterator =
          target_table_value->find(update_data_iterator.first);

      if (target_item_iterator == target_table_value->end()) {
        // target did not have this new key
        return true;
      }

      lepus::Value target_item_value = target_item_iterator->second;
      lepus::Value update_item_value = update_data_iterator.second;

      if (CheckTableValueNotEqual(target_item_value, update_item_value)) {
        return true;
      }
    }

    return false;
  } else {
    return target != update;
  }
}

void ForEachLepusValue(const lepus::Value& value,
                       lepus::LepusValueIterator func) {
  if (value.IsJSValue()) {
    value.IteratorJSValue(std::move(func));
    return;
  }

  switch (value.Type()) {
    case lepus::ValueType::Value_Table: {
      auto value_scope_ref_ptr = value.Table();
      auto& table = *value_scope_ref_ptr;
      for (auto& pair : table) {
        auto key = lepus::Value(pair.first);
        func(key, pair.second);
      }
    } break;
    case lepus::ValueType::Value_Array: {
      auto value_scope_ref_ptr = value.Array();
      auto& array = *value_scope_ref_ptr;
      for (auto i = decltype(array.size()){}; i < array.size(); ++i) {
        func(lepus::Value{static_cast<int64_t>(i)}, array.get(i));
      }
    } break;
    default: {
      func(lepus::Value{}, value);
    } break;
  }
}

std::string GetTimingFlag(const lepus_value& table) {
  if (table.IsObject()) {
    BASE_STATIC_STRING_DECL(kLynxTimingFlag, "__lynx_timing_flag");
    return table.GetProperty(kLynxTimingFlag).StdString();
  }
  return "";
}

lepus::Value ConvertJSValueToLepusValue(const lepus::Value& value) {
  lepus::Value result;
  if (value.IsJSString()) {
    result.SetString(value.String());
  } else if (value.IsJSBool()) {
    result.SetBool(value.Bool());
  } else if (value.IsJSInteger()) {
    result.SetNumber((value.Int64()));
  } else if (value.IsJSNumber()) {
    result.SetNumber(value.Number());
  } else if (value.IsArray() || value.IsJSArray()) {
    auto array = value.Array();
    tasm::ForEachLepusValue(
        value, [&array](const lepus::Value& key, const lepus::Value& val) {
          array->set(key.Number(), ConvertJSValueToLepusValue(val));
        });
    result.SetArray(std::move(array));
  } else if (value.IsTable() || value.IsJSTable()) {
    auto dic = value.Table();
    tasm::ForEachLepusValue(
        value, [&dic](const lepus::Value& key, const lepus::Value& val) {
          dic->SetValue(key.String(), ConvertJSValueToLepusValue(val));
        });
    result.SetTable(std::move(dic));
  } else {
    result = value;
  }
  return result;
}

// TODO(kechenglong): impl ToLepusValue in PipelineOptions.
lepus::Value PipelineOptionsToLepusValue(
    const PipelineOptions& pipeline_options) {
  lepus::Value pipeline_options_obj = lepus::Value::CreateObject();
  pipeline_options_obj.SetProperty(BASE_STATIC_STRING(kPipelineID),
                                   lepus::Value(pipeline_options.pipeline_id));
  pipeline_options_obj.SetProperty(
      BASE_STATIC_STRING(kPipelineOrigin),
      lepus::Value(pipeline_options.pipeline_origin));
  pipeline_options_obj.SetProperty(
      BASE_STATIC_STRING(kPipelineNeedTimestamps),
      lepus::Value(pipeline_options.need_timestamps));
  return pipeline_options_obj;
}

}  // namespace tasm
}  // namespace lynx
