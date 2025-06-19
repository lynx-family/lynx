// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_VALUE_PATH_PARSER_H_
#define BASE_INCLUDE_VALUE_PATH_PARSER_H_

#include <string>

#include "base/include/vector.h"

namespace lynx {
namespace lepus {

using PathVector = base::InlineVector<std::string, 8>;

// handle value path
// a.b => [a, b]
// a[3] => [a, 3]
// returns an empty vector if path is invalid
PathVector ParseValuePath(const std::string &path);

}  // namespace lepus
}  // namespace lynx

#endif  // BASE_INCLUDE_VALUE_PATH_PARSER_H_
