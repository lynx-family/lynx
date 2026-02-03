// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/common/attribute_utils.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <string>

#include "base/include/compiler_specific.h"
#include "base/include/fml/macros.h"
#include "base/include/string/quickjs_dtoa.h"
#include "clay/public/clay.h"
#include "clay/public/value.h"
#include "clay/ui/common/value_utils.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/page_view.h"

namespace clay {
namespace attribute_utils {

#define RETURN_DEFAULT_IF_NULL(value, def_val) \
  do {                                         \
    if (IsNullOrInvalid(value)) {              \
      return def_val;                          \
    }                                          \
  } while (false)

template <typename T>
T GetNumber(const clay::Value& value, T default_val = 0) {
  RETURN_DEFAULT_IF_NULL(value, default_val);
  if (value.IsInt()) {
    return value.GetInt();
  } else if (value.IsUint()) {
    return value.GetUint();
  } else if (value.IsLong()) {
    return value.GetLong();
  } else if (value.IsDouble()) {
    return value.GetDouble();
  } else if (value.IsFloat()) {
    return value.GetFloat();
  } else if (value.IsString()) {
    double v;
    if (lynx::base::StringToDouble(value.GetString(), v)) {
      return v;
    } else {
      FML_DLOG(ERROR) << "Failed convert " << value.GetString() << " to double";
    }
  } else {
    FML_UNREACHABLE();
  }
  return default_val;
}

double GetNum(const clay::Value& value, double default_val) {
  return GetNumber(value, default_val);
}

int GetInt(const clay::Value& value, int default_val) {
  return GetNumber(value, default_val);
}

uint32_t GetUint(const clay::Value& value, uint32_t default_val) {
  return GetNumber(value, default_val);
}

int64_t GetLong(const clay::Value& value, int64_t default_val) {
  return GetNumber(value, default_val);
}

// Lynx has no float value
double GetDouble(const clay::Value& value, double default_val) {
  return GetNumber(value, default_val);
}

std::string GetCString(const clay::Value& value, std::string default_val) {
  RETURN_DEFAULT_IF_NULL(value, default_val);
  std::string str;
  switch (value.type()) {
    case clay::Value::kString: {
      str = value.GetString();
      break;
    }
    case clay::Value::kInt:
    case clay::Value::kUInt:
    case clay::Value::kLong:
    case clay::Value::kFloat:
    case clay::Value::kDouble: {
      char tmp[128];
      js_dtoa(tmp, GetNumber<double>(value));
      str = tmp;
      break;
    }
    case clay::Value::kBool: {
      str = value.GetBool() ? "true" : "false";
      break;
    }
    case clay::Value::kPointer:
    default:
      FML_DLOG(ERROR) << "clay::Value to string not support type";
      break;
  }
  return str;
}

bool GetBool(const clay::Value& value, bool default_val) {
  RETURN_DEFAULT_IF_NULL(value, default_val);
  if (LIKELY(value.IsBool())) {
    return value.GetBool();
  } else if (value.IsString()) {
    auto& str = value.GetString();
    if (str == attr_value::kTrue || str == attr_value::kYes) {
      return true;
    } else if (str == attr_value::kFalse || str == attr_value::kNo) {
      return false;
    } else {
      return default_val;
    }
  } else if (value.IsPointer()) {
    return (value.GetPointer() != nullptr);
  } else {
    double res = 0.0;
    res = GetNumber(value, res /*default value*/);
    return (res > 0.0);
  }
}

bool TryGetNum(const clay::Value& value, double& result, double default_val) {
  if (IsNull(value)) {
    result = default_val;
    return true;
  } else if (value.IsInt()) {
    result = static_cast<double>(value.GetInt());
    return true;
  } else if (value.IsUint()) {
    result = static_cast<double>(value.GetUint());
    return true;
  } else if (value.IsLong()) {
    result = static_cast<double>(value.GetLong());
    return true;
  } else if (value.IsFloat()) {
    result = static_cast<double>(value.GetFloat());
    return true;
  } else if (value.IsDouble()) {
    result = value.GetDouble();
    return true;
  } else if (value.IsString()) {
    if (lynx::base::StringToDouble(value.GetString(), result)) {
      return true;
    }
  }
  return false;
}

bool TryGetString(const clay::Value& value, std::string& result,
                  std::string default_val) {
  if (IsNull(value)) {
    result = default_val;
  } else {
    result = GetCString(value, default_val);
  }
  if (result.empty()) {
    return false;
  }
  return true;
}

static bool CheckLengthUnit(const std::string& input_copy, char* endptr,
                            Unit& unit) {
  if (input_copy.c_str() + input_copy.size() == endptr) {
    unit = Unit::kNone;
    return true;
  }

  if (!endptr) {
    return false;
  }

  std::string str_endptr(endptr, endptr + strlen(endptr));
  str_endptr = lynx::base::TrimString(str_endptr);
  if (str_endptr == "px") {
    unit = Unit::kPx;
    return true;
  }
  if (str_endptr == "ppx") {
    unit = Unit::kPpx;
    return true;
  }
  if (str_endptr == "rpx") {
    unit = Unit::kRpx;
    return true;
  }
  if (str_endptr == "em") {
    unit = Unit::kEm;
    return true;
  }
  if (str_endptr == "rem") {
    unit = Unit::kRem;
    return true;
  }
  if (str_endptr == "%") {
    unit = Unit::kPercent;
    return true;
  }
  return false;
}

// Widgets like <x-input> may send string as value like " 50 px ",
// so we should get the value with unit and transform the value
// after use `ToPxWithDisplayMetrics`
bool TryGetLength(const clay::Value& value, Length& result,
                  Length default_val) {
  if (IsNull(value)) {
    result = default_val;
    return true;
  }
  // when lynx sends value like " 50px ", it is a string type
  if (value.IsString()) {
    std::string input;
    if (!TryGetString(value, input)) {
      return false;
    }
    result.val = 0.0;
    std::string input_copy = lynx::base::TrimString(input);
    char* endptr = nullptr;
    errno = 0;
    double d = strtod(input_copy.c_str(), &endptr);
    Unit unit = Unit::kOther;
    bool valid = (errno == 0 && !input_copy.empty() &&
                  CheckLengthUnit(input_copy, endptr, unit));
    if (valid) {
      result.val = d;
      result.unit = unit;
    }
    return valid;
  } else if (value.IsDouble() || value.IsInt() || value.IsUint() ||
             value.IsFloat() || value.IsLong()) {
    result.unit = Unit::kNone;
    return TryGetNum(value, result.val, default_val.val);
  }
  return false;
}

bool TryGetPlatformLength(const clay::Value::Array& array, const int index,
                          ClayPlatformLength& result,
                          ClayPlatformLength default_val) {
  int len = array.size();
  if (index + 1 >= len) {
    result = default_val;
    return false;
  }
  if (!array[index].IsDouble() || !array[index + 1].IsInt()) {
    result = default_val;
    return false;
  }
  result.value = GetDouble(array[index]);
  result.unit = static_cast<ClayPlatformLengthUnit>(GetInt(array[index + 1]));
  return true;
}

float ResolvePlatformLength(const ClayPlatformLength& length, float size) {
  return length.unit == ClayPlatformLengthUnit::kNumber ? length.value
                                                        : length.value * size;
}

const clay::Value::Array& GetArray(const clay::Value& value) {
  static const clay::Value::Array default_val;
  RETURN_DEFAULT_IF_NULL(value, default_val);
  FML_DCHECK(value.IsArray());
  if (!value.IsArray()) {
    return default_val;
  }
  return value.GetArray();
}

const clay::Value::Map& GetMap(const clay::Value& value) {
  static const clay::Value::Map default_val{};
  RETURN_DEFAULT_IF_NULL(value, default_val);
  FML_DCHECK(value.IsMap());
  if (!value.IsMap()) {
    return default_val;
  }
  return value.GetMap();
}

const clay::Value& GetMapItem(const clay::Value::Map& map,
                              const std::string& key) {
  const auto it = map.find(key);
  if (it != map.end()) {
    return it->second;
  }
  return GetNullClayValue();
}

Point GetPoint(const clay::Value& value) {
  Point point;
  if (!value.IsString()) {
    return point;
  }
  std::string_view view(value.GetString());

  size_t pos = view.find(",");
  if (pos == std::string_view::npos || pos >= view.size() - 1) {
    return point;
  }

  point.SetX(atoi(view.data()));
  point.SetY(atoi(view.data() + pos + 1));
  return point;
}

// Get the resolved length value in clay pixels. Currently supports px, rpx, ppx
float ToPxWithDisplayMetrics(const Length& value_with_unit,
                             const PageView* page_view, float view_length) {
  double value = value_with_unit.val;
  Unit unit = value_with_unit.unit;
  switch (unit) {
    case Unit::kRpx:
      return page_view->ConvertFrom<kPixelTypePhysical>(
          page_view->physical_size().width() * value / 750.f);
    case Unit::kPpx:
      return page_view->ConvertFrom<kPixelTypePhysical>(value);
    case Unit::kPercent:
      return std::max(0.f, view_length) * value / 100.f;
    // TODO(feiyue): temporarily we treat em, rem and other as px
    // because em and rem need more info from lynx side and now clay
    // cannot transform these types
    case Unit::kPx:
    default:
      return page_view->ConvertFrom<kPixelTypeLogical>(value);
  }
}

// Get the resolved length value in clay pixels. Currently supports px, rpx, ppx
float ToPxWithDisplayMetrics(const std::string& value_with_unit,
                             const PageView* page_view) {
  auto value = clay::Value(value_with_unit);
  Length result;
  // if TryGetLength return false, it will set result
  // to a default value where result.val will be 0.0
  TryGetLength(value, result);
  return ToPxWithDisplayMetrics(result, page_view);
}

#ifndef NDEBUG
std::string ToString(const clay::Value& value) {
  switch (value.type()) {
    case clay::Value::kNone:
      return "none";
    case clay::Value::kBool:
      return value.GetBool() ? "true" : "false";
    case clay::Value::kInt:
      return std::string("I:") + std::to_string(value.GetInt());
    case clay::Value::kUInt:
      return std::string("U:") + std::to_string(value.GetUint());
    case clay::Value::kLong:
      return std::string("L:") + std::to_string(value.GetLong());
    case clay::Value::kFloat:
      return std::string("F:") + std::to_string(value.GetFloat());
    case clay::Value::kDouble:
      return std::string("D:") + std::to_string(value.GetDouble());
    case clay::Value::kString:
      return value.GetString();
    case clay::Value::kPointer: {
      std::stringstream ss;
      ss << "P:" << value.GetPointerType() << " 0x" << value.GetPointer();
      return ss.str();
    }
    case clay::Value::kArray: {
      std::stringstream ss;
      const auto& array = value.GetArray();
      ss << "Array[" << array.size() << "]:{\n";
      for (size_t i = 0; i < array.size(); ++i) {
        ss << ToString(array[i]) << "\n";
      }
      ss << "}";
      return ss.str();
    }
    case clay::Value::kArrayBuffer: {
      std::stringstream ss;
      const auto& buf = value.GetArrayBuffer();
      ss << "ArrayBuffer[" << buf.size() << "]";
      for (size_t i = 0; i < buf.size(); ++i) {
        ss << " " << std::setfill('0') << std::setw(2) << std::hex
           << buf.data()[i];
      }
      return ss.str();
    }
    case clay::Value::kMap: {
      const auto& map = GetMap(value);
      std::stringstream ss;
      ss << "Map[" << map.size() << "]:{\n";
      for (const auto& pair : map) {
        ss << pair.first << " : " << ToString(pair.second) << "\n";
      }
      ss << "}";
      return ss.str();
    }
  }
}

#endif

}  // namespace attribute_utils
}  // namespace clay
