// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_ICU_UTIL_H_
#define CLAY_FML_ICU_UTIL_H_

#include <memory>
#include <string>

#include "base/include/fml/macros.h"
#include "clay/fml/mapping.h"

namespace fml {
namespace icu {

void InitializeICU(const std::string& icu_data_path = "");

void InitializeICUFromMapping(std::unique_ptr<Mapping> mapping);

}  // namespace icu
}  // namespace fml

#endif  // CLAY_FML_ICU_UTIL_H_
