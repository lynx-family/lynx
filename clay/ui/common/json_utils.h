// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_JSON_UTILS_H_
#define CLAY_UI_COMMON_JSON_UTILS_H_

#include <optional>

namespace clay {

namespace json_utils {

template <typename T, typename K, typename Object>
std::optional<T> TryGetObject(Object& obj, K key) {
  auto iter = obj.FindMember(key);
  if (iter == obj.MemberEnd()) {
    return {};
  }
  if (!iter->value.template Is<T>()) {
    return {};
  }
  return iter->value.template Get<T>();
}

template <typename T, typename Array>
std::optional<T> TryGetArray(Array& arr, uint32_t index) {
  if (index >= arr.Size()) {
    return {};
  }
  if (!arr[index].template Is<T>()) {
    return {};
  }
  return arr[index].template Get<T>();
}

}  // namespace json_utils

}  // namespace clay

#endif  // CLAY_UI_COMMON_JSON_UTILS_H_
