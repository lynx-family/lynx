// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <sstream>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/sorted_for_each.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/include/value/lynx_value_extended.h"
#include "base/include/value/table.h"

/// TODO(yuyang). The two Print methods of Value are moved to this separate file
/// to temporarily solve the random code degradation problem caused by the "-Oz"
/// compiler option. The degraded code is in the `Value::Copy` method, and the
/// compiler will insert unexpected outlined function.

namespace lynx {
namespace lepus {

void Value::Print() const {
  std::ostringstream s;
  PrintValue(s);
  LOGE(s.str() << std::endl);
}

void Value::PrintValue(std::ostream& output, bool ignore_other, bool pretty,
                       bool sort_map_key) const {
  if (IsJSValue()) {
    lynx_value_print_ext(env_, value_, &output, nullptr);
    return;
  }
  int64_t i64;
  uint64_t u64;
  switch (value_.type) {
    case lynx_value_null:
      if (!ignore_other) {
        output << "null";
      }
      break;
    case lynx_value_undefined:
      if (!ignore_other) {
        output << "undefined";
      }
      break;
    case lynx_value_double:
      output << base::StringConvertHelper::DoubleToString(Number());
      break;
    case lynx_value_int32:
      i64 = Int32();
      goto write_i64;
    case lynx_value_int64:
      i64 = Int64();
    write_i64:
      output << i64;
      break;
    case lynx_value_uint32:
      u64 = UInt32();
      goto write_u64;
    case lynx_value_uint64:
      u64 = UInt64();
    write_u64:
      output << u64;
      break;
    case lynx_value_bool:
      output << (Bool() ? "true" : "false");
      break;
    case lynx_value_string:
      if (pretty) {
        output << "\"" << CString() << "\"";
      } else {
        output << CString();
      }
      break;
    case lynx_value_map: {
      auto table = Table();
      output << "{";

      bool first = true;
      auto print_each = [&](const auto& it) {
        if (!first) {
          output << ",";
        }
        if (pretty) {
          output << "\"" << it.first.str() << "\"";
        } else {
          output << it.first.str();
        }
        output << ":";
        it.second.PrintValue(output, ignore_other, pretty, sort_map_key);
        first = false;
      };

      if (sort_map_key) {
        base::sorted_for_each(
            table->begin(), table->end(),
            [&](const auto& it) { print_each(it); },
            [](const auto& left, const auto& right) {
              return left.first.str() < right.first.str();
            });
      } else {
        for (const auto& it : *table) {
          print_each(it);
        }
      }
      output << "}";
      break;
    }
    case lynx_value_array: {
      auto array = Array();
      output << "[";
      for (size_t i = 0; i < array->size(); i++) {
        array->get(i).PrintValue(output, ignore_other, pretty, sort_map_key);
        if (i != (array->size() - 1)) {
          output << ",";
        }
      }
      output << "]";
      break;
    }
    case lynx_value_function:
    case lynx_value_external:
      if (!ignore_other) {
        output << "closure/cfunction/cpointer/refcounted" << std::endl;
      }
      break;
    case lynx_value_object: {
      RefType ref_type = static_cast<RefType>(value_.tag);
      switch (ref_type) {
        case RefType::kJSIObject:
          if (!ignore_other) {
            RefCounted()->Print(output);
          }
          break;
#if !ENABLE_JUST_LEPUSNG
        case RefType::kClosure:
          if (!ignore_other) {
            output << "closure/cfunction/cpointer/refcounted" << std::endl;
          }
          break;
        case RefType::kCDate:
          if (!ignore_other) {
            RefCounted()->Print(output);
          }
          break;
        case RefType::kRegExp: {
          RefCounted()->Print(output);
        } break;
#endif
        default:
          // RefCounted
          if (!ignore_other) {
            output << "closure/cfunction/cpointer/refcounted" << std::endl;
          }
          break;
      }
    } break;
    case lynx_value_nan:
      if (!ignore_other) {
        output << "NaN";
      }
      break;
    case lynx_value_arraybuffer:
      if (!ignore_other) {
        output << "ByteArray";
      }
      break;
    default:
      if (!ignore_other) {
        output << "unknow type";  // typo
      }
      break;
  }
}

}  // namespace lepus
}  // namespace lynx
