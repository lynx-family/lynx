// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_LIST_COMPONENT_INFO_H_
#define CORE_RENDERER_DOM_LIST_COMPONENT_INFO_H_

#include <string>

#include "core/renderer/dom/vdom/radon/radon_dispatch_option.h"
#include "core/renderer/starlight/types/layout_attribute.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

struct ListComponentInfo {
  enum class Type : uint32_t {
    DEFAULT = static_cast<uint32_t>(starlight::ListComponentType::DEFAULT),
    HEADER = static_cast<uint32_t>(starlight::ListComponentType::HEADER),
    FOOTER = static_cast<uint32_t>(starlight::ListComponentType::FOOTER),
    LIST_ROW = static_cast<uint32_t>(starlight::ListComponentType::LIST_ROW),
  };

  constexpr static const char kListCompType[] = "list-comp-type";

  ListComponentInfo(const std::string& name, const std::string& current_entry,
                    const lepus::Value& data, const lepus::Value& properties,
                    const lepus::Value& ids, const lepus::Value& style,
                    const lepus::Value& clazz, const lepus::Value& event,
                    const lepus::Value& dataset, const lepus::Value& comp_type);

  friend bool operator==(const ListComponentInfo& lhs,
                         const ListComponentInfo& rhs);
  friend bool operator!=(const ListComponentInfo& lhs,
                         const ListComponentInfo& rhs);
  bool CanBeReusedBy(const ListComponentInfo& rhs) const;

  static bool IsRow(uint32_t type) {
    return type == (uint32_t)Type::HEADER || type == (uint32_t)Type::FOOTER ||
           type == (uint32_t)Type::LIST_ROW;
  }

  bool IsEqualWithoutPropsId(const ListComponentInfo& rhs) const;

  std::string current_entry_;
  lepus::Value diff_key_;
  // Deprecated: using estimated_main_axis_size_px_
  double estimated_height_;
  // Deprecated: using estimated_main_axis_size_px_
  double estimated_height_px_;
  double estimated_main_axis_size_px_;
  bool stick_top_;
  bool stick_bottom_;
  bool no_valid_item_key_;
  ListComponentDispatchOption list_component_dispatch_option_;
  std::string name_;
  lepus::Value data_;
  lepus::Value properties_;
  lepus::Value ids_;
  lepus::Value style_;
  lepus::Value clazz_;
  lepus::Value event_;
  lepus::Value dataset_;
  lepus::Value lepus_type_;
  lepus::Value lepus_name_;
  ListComponentInfo::Type type_;
  int distance_from_root_{0};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_LIST_COMPONENT_INFO_H_
