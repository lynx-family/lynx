// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_LYNX_MODULE_TYPE_UTILS_H_
#define CLAY_UI_LYNX_MODULE_TYPE_UTILS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/lynx_module/types.h"

namespace clay {

// Cast a `clay::Value` to a c++ value. The default `Cast()` is not implemented.
// Define a specialization to define the casting behaviour.
template <typename DstType>
struct LynxModuleArgCastor {
  static void Cast(const clay::Value& value, DstType& dst_value) = delete;
};

template <>
struct LynxModuleArgCastor<bool> {
  static void Cast(const clay::Value& value, bool& dst_value);
};

template <>
struct LynxModuleArgCastor<int32_t> {
  static void Cast(const clay::Value& value, int32_t& dst_value);
};

template <>
struct LynxModuleArgCastor<uint32_t> {
  static void Cast(const clay::Value& value, uint32_t& dst_value);
};

template <>
struct LynxModuleArgCastor<float> {
  static void Cast(const clay::Value& value, float& dst_value);
};

template <>
struct LynxModuleArgCastor<std::string> {
  static void Cast(const clay::Value& value, std::string& dst_value);
};

template <>
struct LynxModuleArgCastor<LynxModuleValues> {
  static void Cast(const clay::Value& value, LynxModuleValues& dst_value);
};

template <>
struct LynxModuleArgCastor<clay::Value::Map> {
  static void Cast(const clay::Value& value, clay::Value::Map& dst_value);
};

template <class T>
struct LynxModuleArgCastor<std::vector<T>> {
  static void Cast(const clay::Value& value, std::vector<T>& dst_value) {
    FML_DCHECK(value.IsArray());
    if (!value.IsArray()) {
      return;
    }
    auto& array = value.GetArray();
    dst_value.resize(array.size());
    for (size_t i = 0; i < array.size(); ++i) {
      LynxModuleArgCastor<T>::Cast(array[i], dst_value[i]);
    }
  }
};

// Casting multiple `clay::Value`s to the destination objects.
// NOTE: You must initialize the dst_values first. If a parsing error occurs,
// the passed value will be used.
template <typename... DstTypes>
bool CastLynxModuleArgs(const LynxModuleValues& values,
                        DstTypes&... dst_values) {
  FML_DCHECK(values.values.size() >= sizeof...(dst_values));
  if (values.values.size() < sizeof...(dst_values)) {
    return false;
  }
  std::size_t i{0};
  ((LynxModuleArgCastor<DstTypes>::Cast(values.values[i++], dst_values)), ...);
  return true;
}

template <typename... DstTypes>
bool CastNamedLynxModuleArgs(const std::vector<std::string>& arg_names,
                             const LynxModuleValues& values,
                             DstTypes&... dst_values) {
  FML_DCHECK(arg_names.size() != 0);
  FML_DCHECK(arg_names.size() == sizeof...(dst_values));
  std::size_t i{0};
  ((LynxModuleArgCastor<DstTypes>::Cast(values.Get(arg_names[i++]),
                                        dst_values)),
   ...);
  return true;
}

}  // namespace clay

#endif  // CLAY_UI_LYNX_MODULE_TYPE_UTILS_H_
