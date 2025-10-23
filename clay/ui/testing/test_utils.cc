// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/testing/test_utils.h"

#include <cstring>
#include <utility>

namespace clay {

bool operator==(const clay::Value& a, const clay::Value& b) {
  if (a.type() != b.type()) {
    return false;
  }
  switch (a.type()) {
    case clay::Value::kNone:
      return true;
    case clay::Value::kBool:
      return a.GetBool() == b.GetBool();
    case clay::Value::kInt:
      return a.GetInt() == b.GetInt();
    case clay::Value::kUInt:
      return a.GetUint() == b.GetUint();
    case clay::Value::kLong:
      return a.GetLong() == b.GetLong();
    case clay::Value::kFloat:
      return a.GetFloat() == b.GetFloat();
    case clay::Value::kDouble:
      return a.GetDouble() == b.GetDouble();
    case clay::Value::kString:
      return a.GetString() == b.GetString();
    case clay::Value::kPointer:
      return a.GetPointer() == b.GetPointer();
    case clay::Value::kArray: {
      if (a.GetArray().size() != b.GetArray().size()) {
        return false;
      }
      for (size_t i = 0; i < a.GetArray().size(); i++) {
        if (!(a.GetArray()[i] == b.GetArray()[i])) {
          return false;
        }
      }
      return true;
    }
    case clay::Value::kArrayBuffer:
      return &a.GetArrayBuffer() == &b.GetArrayBuffer();
    case clay::Value::kMap:
      return &a.GetMap() == &b.GetMap();
  }
}

bool operator!=(const Value& a, const Value& b) { return !(a == b); }

std::ostream& operator<<(std::ostream& stream, const FloatPoint& p) {
  return stream << "(" << p.x() << ", " << p.y() << ")";
}

std::ostream& operator<<(std::ostream& stream, const FloatRect& r) {
  return stream << "(" << r.x() << ", " << r.y() << ", " << r.width() << ", "
                << r.height() << ")";
}

std::ostream& operator<<(std::ostream& stream, const clay::Value& v) {
  switch (v.type()) {
    case clay::Value::kNone:
      stream << "<none>";
      break;
    case clay::Value::kBool:
      stream << (v.GetBool() ? "true" : "false");
      break;
    case clay::Value::kInt:
      stream << "(int) " << v.GetInt();
      break;
    case clay::Value::kUInt:
      stream << "(uint) " << v.GetUint();
      break;
    case clay::Value::kLong:
      stream << "(int64) " << v.GetLong();
      break;
    case clay::Value::kFloat:
      stream << "(float) " << v.GetFloat();
      break;
    case clay::Value::kDouble:
      stream << "(double) " << v.GetDouble();
      break;
    case clay::Value::kString:
      stream << '"' << v.GetString() << '"';
      break;
    case clay::Value::kPointer:
      stream << "<" << v.GetPointerType() << ": " << v.GetPointer() << ">";
      break;
    case clay::Value::kArray:
      stream << "[";
      for (size_t i = 0; i < v.GetArray().size(); i++) {
        if (i > 0) {
          stream << ", ";
        }
        stream << v.GetArray()[i];
      }
      stream << "]";
      break;
    case clay::Value::kArrayBuffer:
      stream << "<ArrayBuffer: " << v.GetArrayBuffer().data() << "|"
             << v.GetArrayBuffer().size() << ">";
      break;
    case clay::Value::kMap:
      stream << "{";
      const auto& map = v.GetMap();
      bool first = true;
      for (const auto& pair : map) {
        if (!first) {
          stream << ", ";
        }
        first = false;
        stream << pair.first << ": " << pair.second;
      }
      stream << "}";
      break;
  }
  return stream;
}

LynxModuleValues CreateLynxModuleValues(std::vector<std::string>&& names,
                                        std::initializer_list<Value>&& values) {
  LynxModuleValues value;
  value.names = std::move(names);
  for (auto& item : values) {
    value.values.emplace_back(std::move(const_cast<Value&>(item)));
  }
  return value;
}

}  // namespace clay
