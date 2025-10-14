// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/common/value_utils.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace clay {

const clay::Value& GetNullClayValue() {
  static clay::Value value{
      ClayPointer{ClayPointer::kClayPointerTypeVoidPtr, nullptr}};
  return value;
}

clay::Value CloneClayMap(const clay::Value& map) {
  if (!map.IsMap()) {
    return clay::Value::Null();
  }
  clay::Value::Map rk_map;
  for (const auto& [key, value] : map.GetMap()) {
    rk_map.emplace(key, CloneClayValue(value));
  }
  return clay::Value{std::move(rk_map)};
}

clay::Value CloneClayArray(const clay::Value& array) {
  if (!array.IsArray()) {
    return clay::Value::Null();
  }
  auto size = array.GetArray().size();
  clay::Value::Array rk_array(size);
  for (size_t i = 0; i < size; ++i) {
    rk_array[i] = CloneClayValue(array.GetArray().at(i));
  }
  return clay::Value{std::move(rk_array)};
}

clay::Value CloneClayValue(const clay::Value& value) {
  switch (value.type()) {
    case clay::Value::kNone:
      return clay::Value();
    case clay::Value::kInt:
      return clay::Value(value.GetInt());
    case clay::Value::kLong:
      return clay::Value(value.GetLong());
    case clay::Value::kFloat:
      return clay::Value(value.GetFloat());
    case clay::Value::kBool:
      return clay::Value(value.GetBool());
    case clay::Value::kDouble:
      return clay::Value(value.GetDouble());
    case clay::Value::kString:
      return clay::Value(value.GetString());
    case clay::Value::kUInt:
      return clay::Value(value.GetUint());
    case clay::Value::kPointer:
      return clay::Value{
          ClayPointer{value.GetPointerType(), value.GetPointer()}};
    case clay::Value::kArrayBuffer:
      // ignore
      FML_DCHECK(false);
      return clay::Value();
    case clay::Value::kArray:
      return CloneClayArray(value);
    case clay::Value::kMap:
      return CloneClayMap(value);
  }
  FML_DCHECK(false);
  return clay::Value{};
}

bool HasProperty(const clay::Value::Map& m, const char* name) {
  const auto search = m.find(name);
  return search != m.end();
}

bool GetBoolProperty(const clay::Value::Map& m, const char* name) {
  const auto search = m.find(name);
  if (search != m.end()) {
    const auto& val = search->second;
    if (val.IsBool()) {
      return val.GetBool();
    }
    if (val.IsString()) {
      return val.GetString() == "true";
    }
    if (val.IsInt()) {
      return val.GetInt() != 0;
    }
    if (val.IsUint()) {
      return val.GetUint() != 0;
    }
    if (val.IsLong()) {
      return val.GetLong() != 0;
    }
    if (val.IsFloat()) {
      return val.GetFloat() != 0 && !isnan(val.GetFloat());
    }
    if (val.IsDouble()) {
      return val.GetDouble() != 0 && !isnan(val.GetDouble());
    }
  }
  return false;
}

int64_t GetIntProperty(const clay::Value::Map& m, const char* name) {
  const auto search = m.find(name);
  if (search != m.end()) {
    const auto& val = search->second;
    if (val.IsInt()) {
      return val.GetInt();
    }
    if (val.IsString()) {
      return strtoll(val.GetString().c_str(), nullptr, 10);
    }
    if (val.IsUint()) {
      return val.GetUint();
    }
    if (val.IsLong()) {
      return val.GetLong();
    }
    if (val.IsFloat()) {
      return val.GetFloat();
    }
    if (val.IsDouble()) {
      return val.GetDouble();
    }
  }
  return 0;
}

double GetDoubleProperty(const clay::Value::Map& m, const char* name) {
  const auto search = m.find(name);
  if (search != m.end()) {
    const auto& val = search->second;
    if (val.IsDouble()) {
      return val.GetDouble();
    }
    if (val.IsInt()) {
      return val.GetInt();
    }
    if (val.IsString()) {
      return strtod(val.GetString().c_str(), nullptr);
    }
    if (val.IsUint()) {
      return val.GetUint();
    }
    if (val.IsLong()) {
      return val.GetLong();
    }
    if (val.IsFloat()) {
      return val.GetFloat();
    }
  }
  return 0;
}

const char* GetStringProperty(const clay::Value::Map& m, const char* name) {
  const auto search = m.find(name);
  if (search != m.end()) {
    const auto& val = search->second;
    if (val.IsString()) {
      return val.GetString().c_str();
    }
  }
  return nullptr;
}

}  // namespace clay
