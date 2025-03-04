// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <iomanip>
#include <sstream>

#include "third_party/binding/common/value.h"

namespace lynx {
namespace binding {

// std::visit() requires iOS 12.0. Work around it by using no-except version.
#define STD_VISIT std::__variant_detail::__visitation::__variant::__visit_value

template <typename T, typename SFINAEHelper = void>
struct ToJSONHelper {
  static void Convert(const T& value, std::stringstream& ss) {
    // Default to json null.
    ss << "null";
  }
};

template <>
struct ToJSONHelper<bool> {
  static void Convert(bool value, std::stringstream& ss) {
    ss << (value ? "true" : "false");
  }
};

// Numeric types.
template <typename T>
struct ToJSONHelper<
    T, typename std::enable_if<std::is_integral<T>::value ||
                               std::is_floating_point<T>::value>::type> {
  static void Convert(const T& value, std::stringstream& ss) { ss << value; }
};

template <>
struct ToJSONHelper<std::string> {
  static void Convert(const std::string& value, std::stringstream& ss) {
    ss << "\"";
    for (auto c : value) {
      switch (c) {
        case '"':
          ss << "\\\"";
          break;
        case '\\':
          ss << "\\\\";
          break;
        case '\b':
          ss << "\\b";
          break;
        case '\f':
          ss << "\\f";
          break;
        case '\n':
          ss << "\\n";
          break;
        case '\r':
          ss << "\\r";
          break;
        case '\t':
          ss << "\\t";
          break;
        default:
          if ('\x00' <= c && c <= '\x1f') {
            ss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
               << static_cast<int>(c);
          } else {
            ss << c;
          }
      }
    }
    ss << "\"";
  }
};

// Iterative conversion.
template <>
struct ToJSONHelper<Value> {
  static void Convert(const Value& value, std::stringstream& ss) {
    ss << value.ToJSONString();
  }
};

// Arrays.
template <typename T>
struct ToJSONHelper<std::vector<T>> {
  static void Convert(const std::vector<T>& vec, std::stringstream& ss) {
    ss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
      if (i > 0) {
        ss << ",";
      }
      ToJSONHelper<T>::Convert(vec[i], ss);
    }
    ss << "]";
  }
};

template <>
struct ToJSONHelper<Value::DictionaryData> {
  static void Convert(const Value::DictionaryData& dict,
                      std::stringstream& ss) {
    ss << "{";
    const auto& kv = dict.kv;
    for (size_t i = 0; i < kv.size(); ++i) {
      if (i > 0) {
        ss << ",";
      }
      ToJSONHelper<std::string>::Convert(kv[i].first, ss);
      ss << ":";
      ToJSONHelper<Value>::Convert(kv[i].second, ss);
    }
    ss << "}";
  }
};

std::string Value::ToJSONString() const {
  // |data_| are defaulted to bool, so check for no data.
  if (type_ == ValueType::kEmpty || type_ == ValueType::kNull ||
      type_ == ValueType::kUndefined) {
    return "null";
  }
  std::stringstream ss;
  STD_VISIT(
      [&ss](const auto& data) {
        ToJSONHelper<std::decay_t<decltype(data)>>::Convert(data, ss);
      },
      data_);
  return ss.str();
}

}  // namespace binding
}  // namespace lynx
