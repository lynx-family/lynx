// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_VALUE_UTILS_H_
#define CLAY_UI_COMMON_VALUE_UTILS_H_

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/public/value.h"

namespace clay {

const clay::Value& GetNullClayValue();

clay::Value CloneClayValue(const clay::Value& value);
clay::Value CloneClayMap(const clay::Value& map);
clay::Value CloneClayArray(const clay::Value& array);

// Usage:
//  auto map = CreateClayMap({"arg1", "arg2", "arg3", val1, val2, val3});
template <typename... ValueTypes>
clay::Value::Map CreateClayMap(const std::vector<std::string>& arg_names,
                               ValueTypes&&... values) {
  clay::Value::Map map;
  if (arg_names.size() != sizeof...(values)) {
    FML_DCHECK(false);
    FML_LOG(ERROR) << "Arg_names size must equal to values size!";
  } else {
    std::size_t i{0};
    ((map.emplace(arg_names[i++], std::move(values))), ...);
  }
  return map;
}

// Value helpers
bool HasProperty(const clay::Value::Map& m, const char* name);
bool GetBoolProperty(const clay::Value::Map& m, const char* name);
int64_t GetIntProperty(const clay::Value::Map& m, const char* name);
double GetDoubleProperty(const clay::Value::Map& m, const char* name);
const char* GetStringProperty(const clay::Value::Map& m, const char* name);

}  // namespace clay

#endif  // CLAY_UI_COMMON_VALUE_UTILS_H_
