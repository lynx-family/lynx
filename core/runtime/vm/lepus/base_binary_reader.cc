// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/base_binary_reader.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_date.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace lepus {

#if !ENABLE_JUST_LEPUSNG
bool BaseBinaryReader::DeserializeFunction(fml::RefPtr<Function>& parent,
                                           fml::RefPtr<Function>& function) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DeserializeFunction");
  // const value
  DECODE_COMPACT_U32(size);
  function->const_values_.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    DECODE_VALUE_INTO(function->const_values_.emplace_back());
  }

  // instruction
  ERROR_UNLESS(ReadCompactU32(&size));
  function->op_codes_.reserve(size);
  function->debug_line_col_.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    Instruction instruction;
    DECODE_COMPACT_U64(op_code);
    instruction.op_code_ = static_cast<long>(op_code);
    function->AddInstruction(instruction);
  }

  // up value info
  DECODE_COMPACT_U32(update_value_size);
  function->upvalues_.reserve(update_value_size);
  for (size_t i = 0; i < update_value_size; ++i) {
    DECODE_STR(name);
    DECODE_COMPACT_U64(reg);
    DECODE_BOOL(in_parent_var);
    function->AddUpvalue(std::move(name), static_cast<long>(reg),
                         in_parent_var);
  }

  // switch info
  const char* version = compile_options_.target_sdk_version_.c_str();
  if (version &&
      lynx::tasm::Config::IsHigherOrEqual(version, FEATURE_CONTROL_VERSION_2)) {
    DECODE_COMPACT_U32(switch_info_size);
    function->switches_.reserve(switch_info_size);
    for (size_t i = 0; i < switch_info_size; ++i) {
      DECODE_COMPACT_U64(key_type);
      DECODE_COMPACT_U64(min);
      DECODE_COMPACT_U64(max);
      DECODE_COMPACT_U64(default_offset);
      DECODE_COMPACT_U64(switch_table_size);
      DECODE_COMPACT_U64(type);
      std::vector<std::pair<long, long>> vec;
      vec.reserve(switch_table_size);
      for (size_t j = 0; j < switch_table_size; j++) {
        DECODE_COMPACT_U64(v1);
        DECODE_COMPACT_U64(v2);
        vec.emplace_back(std::pair<long, long>{v1, v2});
      }
      function->AddSwitch(static_cast<long>(key_type), static_cast<long>(min),
                          static_cast<long>(max),
                          static_cast<long>(default_offset),
                          static_cast<SwitchType>(type), vec);
    }
  }

  func_vec.push_back(function);

  // children
  ERROR_UNLESS(ReadCompactU32(&size));
  function->child_functions_.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    DECODE_FUNCTION(function, child_function);
  }

  if (parent.get() != nullptr) {
    parent->AddChildFunction(function);
  }
  return true;
}

bool BaseBinaryReader::DeserializeGlobal(
    std::unordered_map<base::String, lepus::Value>& global) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DeserializeGlobal");
  DECODE_COMPACT_U32(size);
  for (size_t i = 0; i < size; ++i) {
    DECODE_STR(name);
    DECODE_VALUE_INTO(global[std::move(name)]);
  }
  return true;
}

bool BaseBinaryReader::DeserializeTopVariables(
    std::unordered_map<base::String, long>& top_level_variables) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DeserializeTopVariables");
  DECODE_COMPACT_U32(top_size);
  for (size_t i = 0; i < top_size; i++) {
    DECODE_STR(str);
    DECODE_COMPACT_S32(pos);
    top_level_variables.emplace(std::move(str), pos);
  }
  return true;
}

bool BaseBinaryReader::DecodeClosure(fml::RefPtr<Closure>& out_value) {
  uint32_t value_count = 0;
  ERROR_UNLESS(ReadCompactU32(&value_count));
  uint32_t index = 0;
  ERROR_UNLESS(ReadCompactU32(&index));
  ERROR_UNLESS(index < func_vec.size());
  out_value->function_ = func_vec[index];
  return true;
}

bool BaseBinaryReader::DecodeRegExp(fml::RefPtr<RegExp>& reg) {
  DECODE_VALUE(pattern);
  DECODE_VALUE(flags);
  reg->set_pattern(pattern.String());
  reg->set_flags(flags.String());
  return true;
}

bool BaseBinaryReader::DecodeDate(fml::RefPtr<CDate>& date) {
  tm_extend date_;
  DECODE_COMPACT_S32(language);
  DECODE_COMPACT_S32(ms_);
  DECODE_COMPACT_S32(tm_year);
  DECODE_COMPACT_S32(tm_mon);
  DECODE_COMPACT_S32(tm_mday);
  DECODE_COMPACT_S32(tm_hour);
  DECODE_COMPACT_S32(tm_min);
  DECODE_COMPACT_S32(tm_sec);
  DECODE_COMPACT_S32(tm_wday);
  DECODE_COMPACT_S32(tm_yday);
  DECODE_COMPACT_S32(tm_isdst);
  DECODE_DOUBLE(tm_gmtoff);

  date_.tm_year = tm_year;
  date_.tm_mon = tm_mon;
  date_.tm_mday = tm_mday;
  date_.tm_hour = tm_hour;
  date_.tm_min = tm_min;
  date_.tm_sec = tm_sec;
  date_.tm_wday = tm_wday;
  date_.tm_yday = tm_yday;
  date_.tm_isdst = tm_isdst;
  date_.tm_gmtoff = tm_gmtoff;

  date->SetDate(date_, ms_, language);
  return true;
}
#endif

bool BaseBinaryReader::DeserializeStringSection() { return true; }

bool BaseBinaryReader::DecodeUtf8Str(base::String& result) {
  ReadStringDirectly(result);
  return true;
}

bool BaseBinaryReader::DecodeUtf8Str(std::string* result) {
  ReadStringDirectly(result);
  return true;
}

bool BaseBinaryReader::DecodeTable(fml::RefPtr<Dictionary>& out_value,
                                   bool is_header) {
  DECODE_COMPACT_U32(size);
  for (size_t i = 0; i < size; ++i) {
    // If encode happens in parsing header stage, nothing in string_list, so
    // read string directly
    if (is_header) {
      std::string key;
      ERROR_UNLESS(ReadStringDirectly(&key));
      DECODE_VALUE_HEADER_INTO(*out_value->At(std::move(key)));
    } else {
      DECODE_STR(key);
      DECODE_VALUE_INTO(*out_value->At(std::move(key)));
    }
  }
  return true;
}

bool BaseBinaryReader::DecodeArray(fml::RefPtr<CArray>& ary) {
  DECODE_COMPACT_U32(size);
  ary->reserve(size);
  for (size_t i = 0; i < size; i++) {
    DECODE_VALUE_INTO(*ary->push_back_default());
  }
  return true;
}

bool BaseBinaryReader::DecodeValue(Value* result, bool is_header) {
  DECODE_U8(type);
  switch (type) {
    case ValueType::Value_Int32: {
      DECODE_COMPACT_S32(number);
      result->SetNumber(static_cast<int32_t>(number));
    } break;
    case ValueType::Value_UInt32: {
      DECODE_COMPACT_U32(number);
      result->SetNumber(static_cast<uint32_t>(number));
    } break;
    case ValueType::Value_Int64: {
      DECODE_COMPACT_U64(number);
      result->SetNumber(static_cast<int64_t>(number));
    } break;
    case ValueType::Value_Double: {
      DECODE_DOUBLE(number);
      result->SetNumber(number);
    } break;
    case ValueType::Value_Bool: {
      DECODE_BOOL(boolean);
      result->SetBool(boolean);
    } break;
    case ValueType::Value_String: {
      // If encode happens in parsing header stage, nothing in string_list, so
      // read string directly
      if (is_header) {
        std::string temp;
        ERROR_UNLESS(ReadStringDirectly(&temp));
        result->SetString(std::move(temp));
      } else {
        DECODE_STR(str);
        result->SetString(std::move(str));
      }
    } break;
    case ValueType::Value_Table: {
      DECODE_DICTIONARY(table, is_header);
      result->SetTable(std::move(table));
    } break;
    case ValueType::Value_Array: {
      DECODE_ARRAY(ary);
      result->SetArray(std::move(ary));
    } break;
#if !ENABLE_JUST_LEPUSNG
    case ValueType::Value_Closure: {
      DECODE_CLOSURE(closure);
      result->SetClosure(std::move(closure));
    } break;
    case ValueType::Value_CFunction:
    case ValueType::Value_CPointer:
    case ValueType::Value_RefCounted:
      break;
    case ValueType::Value_Nil:
      break;
    case ValueType::Value_Undefined:
      result->SetUndefined();
      break;
    case ValueType::Value_CDate: {
      DECODE_DATE(date);
      result->SetDate(std::move(date));
      break;
    }
    case ValueType::Value_RegExp: {
      DECODE_REGEXP(reg);
      result->SetRegExp(std::move(reg));
      break;
    }
    case ValueType::Value_NaN: {
      DECODE_BOOL(NaaN);
      result->SetNan(NaaN);
      break;
    }
#endif
    default:
      break;
  }
  return true;
}

bool BaseBinaryReader::DecodeContextBundle(ContextBundle* bundle) {
  if (bundle->IsLepusNG()) {
    auto quick_bundle = static_cast<QuickContextBundle*>(bundle);
    do {
      if (!ReadCompactU64(&quick_bundle->lepusng_code_len())) break;
      quick_bundle->lepus_code().resize(quick_bundle->lepusng_code_len());
      if (!ReadData(quick_bundle->lepus_code().data(),
                    static_cast<int>(quick_bundle->lepusng_code_len())))
        break;
      return true;
    } while (false);
    PrintError("Function %s, %d\n", __FUNCTION__, __LINE__);
    return false;
  }
#if !ENABLE_JUST_LEPUSNG
  auto vm_bundle = static_cast<VMContextBundle*>(bundle);
  auto parent = fml::Ref<Function>(nullptr);
  if (DeserializeGlobal(vm_bundle->lepus_root_global()) &&
      DeserializeFunction(parent, vm_bundle->lepus_root_function()) &&
      DeserializeTopVariables(vm_bundle->lepus_top_variables())) {
    return true;
  }
#endif
  PrintError("Function: %s, %d\n", __FUNCTION__, __LINE__);
  return false;
}

std::vector<base::String>& BaseBinaryReader::string_list() {
  return string_list_;
}

}  // namespace lepus
}  // namespace lynx
