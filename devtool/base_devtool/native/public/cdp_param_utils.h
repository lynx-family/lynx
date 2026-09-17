// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_PARAM_UTILS_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_PARAM_UTILS_H_

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

#include "base/include/string/string_number_convert.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace devtool {

// Read a JSON integer representable as int, without coercing other JSON types.
// Both helpers leave result unchanged on failure, including missing/null
// values.
inline bool ReadIntParam(const Json::Value& value, int& result) {
  if ((value.type() != Json::intValue && value.type() != Json::uintValue) ||
      !value.isInt()) {
    return false;
  }
  result = value.asInt();
  return true;
}

// Read a complete decimal integer string (optional sign), representable as int.
// Field-specific constraints such as non-negativity belong to the caller.
inline bool ReadIntStringParam(const Json::Value& value, int& result) {
  if (!value.isString()) {
    return false;
  }
  const std::string text = value.asString();
  // Restrict the first byte before StringToInt's character-classification
  // check.
  if (text.empty() ||
      ((text[0] < '0' || text[0] > '9') && text[0] != '+' && text[0] != '-')) {
    return false;
  }
  int64_t parsed = 0;
  if (!base::StringToInt(text, parsed) ||
      parsed < std::numeric_limits<int>::min() ||
      parsed > std::numeric_limits<int>::max()) {
    return false;
  }
  result = static_cast<int>(parsed);
  return true;
}

// Read a finite JSON number as double, without coercing other JSON types.
inline bool ReadFiniteNumberParam(const Json::Value& value, double& result) {
  if (value.type() != Json::intValue && value.type() != Json::uintValue &&
      value.type() != Json::realValue) {
    return false;
  }
  const double parsed = value.asDouble();
  if (!std::isfinite(parsed)) {
    return false;
  }
  result = parsed;
  return true;
}

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_PARAM_UTILS_H_
