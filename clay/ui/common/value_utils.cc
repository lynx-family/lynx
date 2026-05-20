// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/common/value_utils.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "clay/gfx/geometry/box_shadow_value.h"
#include "clay/gfx/geometry/filter_value.h"
#include "clay/gfx/geometry/transform_raw.h"
#include "clay/gfx/style/length.h"
#include "clay/public/style_types.h"
#include "clay/ui/common/attribute_utils.h"

namespace clay {
namespace {

struct TransformRawDataIndex {
  static constexpr int INDEX_FUNC = 0;
  static constexpr int INDEX_TRANSLATE_0 = 1;
  static constexpr int INDEX_TRANSLATE_0_UNIT = 2;
  static constexpr int INDEX_TRANSLATE_1 = 3;
  static constexpr int INDEX_TRANSLATE_1_UNIT = 4;
  static constexpr int INDEX_TRANSLATE_2 = 5;
  static constexpr int INDEX_TRANSLATE_2_UNIT = 6;
};

}  // namespace

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

const clay::Value* FindMapItem(const clay::Value::Map& map,
                               const std::string& key) {
  const auto it = map.find(key);
  if (it != map.end()) {
    return &it->second;
  }
  return nullptr;
}

int SafeGetInt(const clay::Value* value, int default_value) {
  if (!value) {
    return default_value;
  }
  double result = default_value;
  attribute_utils::TryGetNum(*value, result, default_value);
  return result;
}

bool SafeGetBool(const clay::Value* value, bool default_value) {
  if (!value) {
    return default_value;
  }
  return attribute_utils::GetBool(*value, default_value);
}

std::string SafeGetString(const clay::Value* value, std::string default_value) {
  if (!value) {
    return default_value;
  }
  std::string result = default_value;
  attribute_utils::TryGetString(*value, result, default_value);
  return result;
}

Length ParseLengthValue(const clay::Value& value, const clay::Value& type) {
  LengthUnit length_type =
      static_cast<LengthUnit>(attribute_utils::GetInt(type));
  if (length_type == LengthUnit::kCalc && value.IsArray()) {
    float fixed = 0.f;
    float percent = 0.f;
    const auto& calc_values = value.GetArray();
    if (calc_values.size() % 2 != 0) {
      FML_DLOG(ERROR) << "Invalid calc length array size: "
                      << calc_values.size();
    }
    for (size_t i = 0; i + 1 < calc_values.size(); i += 2) {
      if (!calc_values[i].IsNumber() || !calc_values[i + 1].IsNumber()) {
        FML_DLOG(ERROR) << "Invalid calc length component.";
        continue;
      }
      int raw_unit = attribute_utils::GetInt(calc_values[i + 1]);
      auto unit = static_cast<LengthUnit>(raw_unit);
      if (unit != LengthUnit::kNum && unit != LengthUnit::kPercent) {
        FML_DLOG(ERROR) << "Unsupported calc length unit: " << raw_unit;
        continue;
      }
      auto component =
          static_cast<float>(attribute_utils::GetDouble(calc_values[i]));
      if (unit == LengthUnit::kPercent) {
        percent += component;
      } else {
        fixed += component;
      }
    }
    return Length::Calc(fixed, percent);
  }
  return Length(static_cast<float>(attribute_utils::GetDouble(value)),
                length_type);
}

std::vector<TransformRaw> ParseTransformRawValues(const clay::Value& value) {
  std::vector<TransformRaw> ops;
  if (!value.IsArray()) {
    return ops;
  }
  const auto& items = value.GetArray();
  ops.reserve(items.size());
  for (const auto& item : items) {
    if (!item.IsArray()) {
      continue;
    }
    const auto& arr = item.GetArray();
    if (arr.size() < 7u) {
      continue;
    }
    TransformRaw op{};
    op.type = attribute_utils::GetInt(arr[TransformRawDataIndex::INDEX_FUNC]);
    if (static_cast<ClayTransformType>(op.type) == ClayTransformType::kMatrix ||
        static_cast<ClayTransformType>(op.type) ==
            ClayTransformType::kMatrix3d) {
      if (arr.size() < 17u) {
        continue;
      }
      for (int i = 0; i < 16; ++i) {
        op.matrix[i] = attribute_utils::GetDouble(arr[i + 1]);
      }
    } else {
      op.values[0] =
          ParseLengthValue(arr[TransformRawDataIndex::INDEX_TRANSLATE_0],
                           arr[TransformRawDataIndex::INDEX_TRANSLATE_0_UNIT]);
      op.values[1] =
          ParseLengthValue(arr[TransformRawDataIndex::INDEX_TRANSLATE_1],
                           arr[TransformRawDataIndex::INDEX_TRANSLATE_1_UNIT]);
      op.values[2] =
          ParseLengthValue(arr[TransformRawDataIndex::INDEX_TRANSLATE_2],
                           arr[TransformRawDataIndex::INDEX_TRANSLATE_2_UNIT]);
    }
    ops.emplace_back(op);
  }
  return ops;
}

std::vector<FilterValue> ParseFilterValues(const clay::Value& value) {
  std::vector<FilterValue> values;
  if (!value.IsArray()) {
    return values;
  }
  const auto& ary = value.GetArray();
  for (const auto& item : ary) {
    if (!item.IsArray()) {
      continue;
    }
    const auto& ary1 = item.GetArray();
    if (ary1.size() != 2) {
      continue;
    }
    FilterValue v;
    v.type = attribute_utils::GetInt(ary1[0]);
    v.value = attribute_utils::GetDouble(ary1[1]);
    values.push_back(v);
  }
  return values;
}

std::vector<BoxShadowValue> ParseBoxShadowValues(const clay::Value& value) {
  std::vector<BoxShadowValue> values;
  if (!value.IsArray()) {
    return values;
  }
  const auto& shadows = value.GetArray();
  for (const auto& shadow : shadows) {
    if (!shadow.IsArray()) {
      continue;
    }
    const auto& shadow_ary = shadow.GetArray();
    if (shadow_ary.size() < 6) {
      continue;
    }
    BoxShadowValue v;
    v.h_offset = static_cast<float>(attribute_utils::GetDouble(shadow_ary[0]));
    v.v_offset = static_cast<float>(attribute_utils::GetDouble(shadow_ary[1]));
    v.blur = static_cast<float>(attribute_utils::GetDouble(shadow_ary[2]));
    v.spread = static_cast<float>(attribute_utils::GetDouble(shadow_ary[3]));
    v.option = attribute_utils::GetDouble(shadow_ary[4]);
    v.color = attribute_utils::GetDouble(shadow_ary[5]);
    values.push_back(v);
  }
  return values;
}

}  // namespace clay
