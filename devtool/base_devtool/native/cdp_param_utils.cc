// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/public/cdp_param_utils.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

#include "base/include/string/string_number_convert.h"

namespace lynx {
namespace devtool {

bool ReadIntParam(const Json::Value& value, int& result) {
  if ((value.type() != Json::intValue && value.type() != Json::uintValue) ||
      !value.isInt()) {
    return false;
  }
  result = value.asInt();
  return true;
}

bool ReadIntStringParam(const Json::Value& value, int& result) {
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

bool ReadFiniteNumberParam(const Json::Value& value, double& result) {
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

bool ReadFiniteFloatParam(const Json::Value& value, float& result) {
  double parsed = 0.0;
  if (!ReadFiniteNumberParam(value, parsed) ||
      parsed < -static_cast<double>(std::numeric_limits<float>::max()) ||
      parsed > static_cast<double>(std::numeric_limits<float>::max())) {
    return false;
  }
  result = static_cast<float>(parsed);
  return true;
}

}  // namespace devtool
}  // namespace lynx
