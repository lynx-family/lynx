// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/string/string_number_convert.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include <cmath>

namespace lynx {
namespace base {

bool StringToDouble(const std::string& input, double& output,
                    bool error_on_nan_or_inf) {
  int old_error = errno;
  errno = 0;
  char* endptr = nullptr;
  double d = strtod(input.c_str(), &endptr);
  bool valid = (errno == 0 && !input.empty() &&
                input.c_str() + input.length() == endptr && !isspace(input[0]));

  if (errno == 0) errno = old_error;
  if (valid) output = d;
  if (error_on_nan_or_inf && (std::isnan(d) || std::isinf(d))) {
    valid = false;
  }
  return valid;
}

bool StringToFloat(const std::string& input, float& output,
                   bool error_on_nan_or_inf) {
  int old_error = errno;
  errno = 0;
  char* endptr = nullptr;
  float f = strtof(input.c_str(), &endptr);
  bool valid = (errno == 0 && !input.empty() &&
                input.c_str() + input.length() == endptr && !isspace(input[0]));
  if (errno == 0) errno = old_error;
  if (valid) output = f;
  if (error_on_nan_or_inf && (std::isnan(f) || std::isinf(f))) {
    valid = false;
  }
  return valid;
}

bool StringToInt(const std::string& input, int64_t& output, uint8_t base) {
  int old_error = errno;
  errno = 0;
  char* endptr = nullptr;
  int64_t i = strtoll(input.c_str(), &endptr, base);
  bool valid = (errno == 0 && !input.empty() &&
                input.c_str() + input.length() == endptr && !isspace(input[0]));
  if (errno == 0) errno = old_error;
  if (valid) output = i;
  return valid;
}

bool StringToInt(const std::string& input, int* output, uint8_t base) {
  int old_error = errno;
  errno = 0;
  char* endptr = nullptr;
  int64_t i = strtoll(input.c_str(), &endptr, base);
  bool valid = (errno == 0 && !input.empty() &&
                input.c_str() + input.length() == endptr && !isspace(input[0]));
  if (errno == 0) errno = old_error;
  if (valid) *output = static_cast<int>(i);
  return valid;
}
}  // namespace base
}  // namespace lynx
