// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_ATTRIBUTE_H_
#define CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_ATTRIBUTE_H_

#include <optional>
#include <string>
#include <unordered_map>

namespace lynx {
namespace starlight {

enum class LayoutAttribute {
  kScroll,
  kColumnCount,
  kListCompType,
  // Indicates that the LayoutNode is list and needs to invoke
  // OnListElementUpdated() when it's layout updated.
  kListContainer,
};

enum class ListComponentType : uint32_t {
  DEFAULT = 0,
  HEADER = 1,
  FOOTER = 2,
  LIST_ROW = 3
};

class AttributesMap {
 public:
  bool setScroll(const std::optional<bool>& newValue) {
    return setAttribute(scroll_, newValue);
  }

  std::optional<bool> getScroll() const { return scroll_; }

  bool setColumnCount(const std::optional<int>& newValue) {
    return setAttribute(column_count_, newValue);
  }

  std::optional<int> getColumnCount() const { return column_count_; }

  bool setListCompType(const std::optional<int>& newValue) {
    return setAttribute(list_comp_type_, newValue);
  }

  std::optional<int> getListCompType() const { return list_comp_type_; }

 private:
  template <typename T>
  bool setAttribute(std::optional<T>& attribute,
                    const std::optional<T>& newValue) {
    if (attribute != newValue) {
      attribute = newValue;
      return true;
    }
    return false;
  }

  std::optional<bool> scroll_;
  std::optional<int> column_count_;
  std::optional<int> list_comp_type_;
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_ATTRIBUTE_H_
