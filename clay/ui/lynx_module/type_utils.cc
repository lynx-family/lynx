// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/lynx_module/type_utils.h"

#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include "clay/fml/logging.h"

namespace clay {

void LynxModuleArgCastor<bool>::Cast(const clay::Value& value,
                                     bool& dst_value) {
  dst_value = attribute_utils::GetBool(value, dst_value);
}

void LynxModuleArgCastor<int32_t>::Cast(const clay::Value& value,
                                        int32_t& dst_value) {
  dst_value = static_cast<int>(attribute_utils::GetNum(value, dst_value));
}

void LynxModuleArgCastor<uint32_t>::Cast(const clay::Value& value,
                                         uint32_t& dst_value) {
  dst_value =
      static_cast<unsigned int>(attribute_utils::GetNum(value, dst_value));
}

void LynxModuleArgCastor<float>::Cast(const clay::Value& value,
                                      float& dst_value) {
  dst_value = static_cast<float>(attribute_utils::GetNum(value, dst_value));
}

void LynxModuleArgCastor<std::string>::Cast(const clay::Value& value,
                                            std::string& dst_value) {
  dst_value = attribute_utils::GetCString(value, dst_value);
}

void LynxModuleArgCastor<LynxModuleValues>::Cast(const clay::Value& value,
                                                 LynxModuleValues& dst_value) {
  FML_DCHECK(value.IsMap());
  if (!value.IsMap()) {
    return;
  }
  dst_value.names.clear();
  dst_value.values.clear();
  auto& mutable_value = const_cast<clay::Value&>(value);
  for (auto& [key, value] : mutable_value.GetMap()) {
    dst_value.names.push_back(key);
    dst_value.values.push_back(std::move(value));
  }
}

void LynxModuleArgCastor<clay::Value::Map>::Cast(const clay::Value& value,
                                                 clay::Value::Map& dst_value) {
  FML_DCHECK(value.IsMap());
  if (!value.IsMap()) {
    return;
  }
  auto& mutable_value = const_cast<clay::Value&>(value);
  dst_value = std::move(mutable_value.GetMap());
}

}  // namespace clay
