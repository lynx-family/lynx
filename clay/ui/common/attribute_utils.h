// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_ATTRIBUTE_UTILS_H_
#define CLAY_UI_COMMON_ATTRIBUTE_UTILS_H_

#include <memory>
#include <sstream>
#include <string>

#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "clay/fml/logging.h"
#include "clay/gfx/geometry/point.h"
#include "clay/gfx/style/color.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"
#include "clay/public/value.h"

namespace clay {

class PageView;

namespace attribute_utils {

/**
 * Compatible with bool-type, string-type, pointer-type, numeric-type.
 * Return default_val when value is none-type.
 */
bool GetBool(const clay::Value& value, bool default_val = false);

enum Unit { kNone, kPx, kPpx, kRpx, kEm, kRem, kOther, kPercent };

/**
we store 50% as {50.0, kPercent}
*/
struct Length {
  double val = 0.0;
  Unit unit = kNone;
};

bool TryGetNum(const clay::Value& value, double& result,
               double default_val = 0);
bool TryGetString(const clay::Value& value, std::string& result,
                  std::string default_val = "");
bool TryGetLength(const clay::Value& value, Length& result,
                  Length default_val = {0.0, Unit::kNone});
bool TryGetPlatformLength(const clay::Value::Array& array, const int index,
                          ClayPlatformLength& result,
                          ClayPlatformLength default_val = {});

float ResolvePlatformLength(const ClayPlatformLength& length, float size);

float ToPxWithDisplayMetrics(const std::string& value_with_unit,
                             const PageView* page_view);

float ToPxWithDisplayMetrics(const Length& value_with_unit,
                             const PageView* page_view, float view_length = -1);

double GetNum(const clay::Value& value, double default_val = 0);
int GetInt(const clay::Value& value, int default_val = 0);
uint32_t GetUint(const clay::Value& value, uint32_t default_val = 0);
int64_t GetLong(const clay::Value& value, int64_t default_val = 0);

// Lynx has no float value
double GetDouble(const clay::Value& value, double default_val = 0);

std::string GetCString(const clay::Value& value, std::string default_val = "");

const clay::Value::Array& GetArray(const clay::Value& value);
const clay::Value::Map& GetMap(const clay::Value& value);
const clay::Value& GetMapItem(const clay::Value::Map&, const std::string& key);

inline Color GetColor(const clay::Value& value,
                      Color default_value = Color::RGBOColor(0, 0, 0, 0)) {
  Color color;
  if (Color::Parse(attribute_utils::GetCString(value), &color)) {
    return color;
  }
  return default_value;
}

// Parse specific format, separated by comma, like "3, -1"
// If parse failed, return {0, 0}
Point GetPoint(const clay::Value& value);

inline bool IsNull(const clay::Value& value) { return value.IsNull(); }

inline bool IsNullOrInvalid(const clay::Value& value) {
  return value.IsNull() || value.IsNone();
}

#ifndef NDEBUG
std::string ToString(const clay::Value& value);
#endif

}  // namespace attribute_utils

}  // namespace clay

#endif  // CLAY_UI_COMMON_ATTRIBUTE_UTILS_H_
