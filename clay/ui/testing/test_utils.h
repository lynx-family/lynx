// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_TESTING_TEST_UTILS_H_
#define CLAY_UI_TESTING_TEST_UTILS_H_

#include <ostream>
#include <string>
#include <vector>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/public/value.h"
#include "clay/ui/lynx_module/types.h"

namespace clay {

bool operator==(const clay::Value& a, const clay::Value& b);
bool operator!=(const clay::Value& a, const clay::Value& b);

// Utility functions for better printing values when assertion failure
std::ostream& operator<<(std::ostream& stream, const FloatPoint& p);
std::ostream& operator<<(std::ostream& stream, const FloatRect& r);
std::ostream& operator<<(std::ostream& stream, const clay::Value& v);

LynxModuleValues CreateLynxModuleValues(
    std::vector<std::string>&& names,
    std::initializer_list<clay::Value>&& values);

}  // namespace clay

#endif  // CLAY_UI_TESTING_TEST_UTILS_H_
