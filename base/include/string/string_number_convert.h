// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_INCLUDE_STRING_STRING_NUMBER_CONVERT_H_
#define BASE_INCLUDE_STRING_STRING_NUMBER_CONVERT_H_

#include <string>

namespace lynx {
namespace base {
bool StringToInt(const std::string& input, int64_t& output, uint8_t base = 10);
bool StringToInt(const std::string& input, int* output, uint8_t base = 10);
/*
 * Convert from string to double.
 * @param input, the string to be converted.
 * @param ouput, output the conversion result, will remain as it was if string
 * is not a valid number.
 * @param error_on_nan_or_inf, treat inf or nan as invalid value if set to true.
 * @return if string is a valid double number.
 */
bool StringToDouble(const std::string& input, double& output,
                    bool error_on_nan_or_inf = false);

bool StringToFloat(const std::string& input, float& output,
                   bool error_on_nan_or_inf = false);
}  // namespace base
}  // namespace lynx
#endif  // BASE_INCLUDE_STRING_STRING_NUMBER_CONVERT_H_
