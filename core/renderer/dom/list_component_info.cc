// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/list_component_info.h"

#include <utility>

#include "core/renderer/utils/value_utils.h"

namespace lynx {
namespace tasm {

constexpr static const char kListItemKey[] = "item-key";
constexpr static const char kListStickyTop[] = "sticky-top";
constexpr static const char kListStickyBottom[] = "sticky-bottom";
constexpr static const char kListEstimatedHeight[] = "estimated-height";
constexpr static const char kListEstimatedHeightPx[] = "estimated-height-px";
constexpr static const char kListEstimatedMainAxisSizePx[] =
    "estimated-main-axis-size-px";
constexpr static double kListEstimatedHeightInvalid = -1.;
constexpr static const char kDistanceFromRoot[] = "distanceFromRoot";

bool operator==(const ListComponentInfo& lhs, const ListComponentInfo& rhs) {
  return (lhs.diff_key_ == rhs.diff_key_) && (lhs.name_ == rhs.name_) &&
         (lhs.ids_ == rhs.ids_) && (lhs.style_ == rhs.style_) &&
         (lhs.clazz_ == rhs.clazz_) && (lhs.event_ == rhs.event_) &&
         (lhs.data_.IsEqual(rhs.data_)) && (lhs.dataset_ == rhs.dataset_) &&
         (lhs.list_component_dispatch_option_ ==
          rhs.list_component_dispatch_option_) &&
         (lhs.IsEqualWithoutPropsId(rhs));
}

bool operator!=(const ListComponentInfo& lhs, const ListComponentInfo& rhs) {
  return !(lhs == rhs);
}

bool ListComponentInfo::IsEqualWithoutPropsId(
    const ListComponentInfo& rhs) const {
  if (properties_.GetLength() != rhs.properties_.GetLength()) {
    return false;
  }

  bool res{true};
  ForEachLepusValue(properties_, [&rhs, &res](const lepus::Value& key,
                                              const lepus::Value& val) {
    if (!res) return;

    auto key_str = key.String();
    if ((key_str.str() != "propsId") &&
        val != rhs.properties_.GetProperty(key_str)) {
      res = false;
    }
  });

  return res;
}

ListComponentInfo::ListComponentInfo(
    const std::string& name, const std::string& current_entry,
    const lepus::Value& data, const lepus::Value& properties,
    const lepus::Value& ids, const lepus::Value& style,
    const lepus::Value& clazz, const lepus::Value& event,
    const lepus::Value& dataset, const lepus::Value& comp_type)
    : current_entry_(current_entry),
      diff_key_{name},
      estimated_height_{kListEstimatedHeightInvalid},
      estimated_height_px_{kListEstimatedHeightInvalid},
      estimated_main_axis_size_px_{kListEstimatedHeightInvalid},
      name_(name),
      data_(data),
      properties_(properties),
      ids_(ids),
      style_(style),
      clazz_(clazz),
      event_(event),
      dataset_(dataset),
      lepus_name_{diff_key_} {
  // extract the item-key from props if available
  // otherwise, use component name as item-key
  auto kListItemKey_str = BASE_STATIC_STRING(kListItemKey);
  if (properties.Contains(kListItemKey_str)) {
    no_valid_item_key_ = false;
    auto item_key = properties.GetProperty(kListItemKey_str);
    if (item_key.IsString() && !item_key.StdString().empty()) {
      diff_key_ = std::move(item_key);
    }
  } else {
    no_valid_item_key_ = true;
  }

  auto kListEstimatedHeight_str = BASE_STATIC_STRING(kListEstimatedHeight);
  if (properties.Contains(kListEstimatedHeight_str)) {
    auto estimated_height = properties.GetProperty(kListEstimatedHeight_str);
    if (estimated_height.IsNumber()) {
      estimated_height_ = estimated_height.Number();
    }
  }

  auto kListEstimatedHeightPx_str = BASE_STATIC_STRING(kListEstimatedHeightPx);
  if (properties.Contains(kListEstimatedHeightPx_str)) {
    auto estimated_height_px =
        properties.GetProperty(kListEstimatedHeightPx_str);
    if (estimated_height_px.IsNumber()) {
      estimated_height_px_ = estimated_height_px.Number();
    }
  }

  auto kListEstimatedMainAxisSizePx_str =
      BASE_STATIC_STRING(kListEstimatedMainAxisSizePx);
  if (properties.Contains(kListEstimatedMainAxisSizePx_str)) {
    auto estimated_main_axis_size_px =
        properties.GetProperty(kListEstimatedMainAxisSizePx_str);
    if (estimated_main_axis_size_px.IsNumber()) {
      estimated_main_axis_size_px_ = estimated_main_axis_size_px.Number();
    }
  }

  const auto& comp_type_str = comp_type.StdString();
  if (comp_type_str == "header") {
    type_ = ListComponentInfo::Type::HEADER;
  } else if (comp_type_str == "footer") {
    type_ = ListComponentInfo::Type::FOOTER;
  } else if (comp_type_str == "list-row") {
    type_ = ListComponentInfo::Type::LIST_ROW;
  } else {
    type_ = ListComponentInfo::Type::DEFAULT;
  }
  lepus_type_ = lepus::Value(static_cast<int32_t>(type_));

  // check the "sticky" props for non-DEFAULT items
  stick_top_ = false;
  stick_bottom_ = false;
  if (type_ != ListComponentInfo::Type::DEFAULT) {
    auto kListStickyTop_str = BASE_STATIC_STRING(kListStickyTop);
    if (properties_.Contains(kListStickyTop_str)) {
      auto value = properties_.GetProperty(kListStickyTop_str);
      stick_top_ = value.IsTrue();
    }
    auto kListStickyBottom_str = BASE_STATIC_STRING(kListStickyBottom);
    if (properties_.Contains(kListStickyBottom_str)) {
      auto value = properties_.GetProperty(kListStickyBottom_str);
      stick_bottom_ = value.IsTrue();
    }
  }

  auto kDistanceFromRoot_str = BASE_STATIC_STRING(kDistanceFromRoot);
  if (properties.Contains(kDistanceFromRoot_str)) {
    distance_from_root_ =
        properties_.GetProperty(kDistanceFromRoot_str).Number();
  }
}

bool ListComponentInfo::CanBeReusedBy(const ListComponentInfo& rhs) const {
  return diff_key_ == rhs.diff_key_;
}
}  // namespace tasm
}  // namespace lynx
