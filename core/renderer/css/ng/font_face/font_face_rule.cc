// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/font_face/font_face_rule.h"

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"

namespace lynx {
namespace css {

lepus_value FontFaceRule::ToLepus() const {
  auto arr = lepus::CArray::Create();

  arr->emplace_back(family_);

  {
    auto src_arr = lepus::CArray::Create();
    for (const auto& src : sources_) {
      auto item = lepus::CArray::Create();
      item->emplace_back(src.is_local);
      item->emplace_back(src.uri);
      src_arr->emplace_back(std::move(item));
    }
    arr->emplace_back(std::move(src_arr));
  }

  arr->emplace_back((static_cast<uint32_t>(weight_min_) << 16) | weight_max_);
  arr->emplace_back((static_cast<uint32_t>(stretch_min_) << 16) | stretch_max_);

  {
    auto style_arr = lepus::CArray::Create();
    style_arr->emplace_back(static_cast<int32_t>(style_.kind));
    style_arr->emplace_back(static_cast<int32_t>(style_.oblique_angle_min));
    style_arr->emplace_back(static_cast<int32_t>(style_.oblique_angle_max));
    arr->emplace_back(std::move(style_arr));
  }

  {
    auto var_arr = lepus::CArray::Create();
    for (const auto& axis : variation_settings_) {
      auto item = lepus::CArray::Create();
      item->emplace_back(axis.tag);
      item->emplace_back(static_cast<double>(axis.value));
      var_arr->emplace_back(std::move(item));
    }
    arr->emplace_back(std::move(var_arr));
  }

  {
    auto range_arr = lepus::CArray::Create();
    for (const auto& range : unicode_range_) {
      auto item = lepus::CArray::Create();
      item->emplace_back(range.from);
      item->emplace_back(range.to);
      range_arr->emplace_back(std::move(item));
    }
    arr->emplace_back(std::move(range_arr));
  }

  return lepus_value(std::move(arr));
}

fml::RefPtr<const FontFaceRule> FontFaceRule::FromLepus(
    const lepus_value& value) {
  if (!value.IsArray()) return nullptr;
  const auto& arr = value.Array();
  if (arr->size() < 7) return nullptr;

  std::string family = arr->get(0).StdString();

  std::vector<FontSource> sources;
  {
    const auto& src_val = arr->get(1);
    if (src_val.IsArray()) {
      const auto& src_arr = src_val.Array();
      sources.reserve(src_arr->size());
      for (size_t i = 0; i < src_arr->size(); ++i) {
        const auto& item = src_arr->get(i);
        if (!item.IsArray() || item.Array()->size() < 2) continue;
        FontSource src;
        src.is_local = item.Array()->get(0).Bool();
        src.uri = item.Array()->get(1).StdString();
        sources.push_back(std::move(src));
      }
    }
  }

  uint32_t weight_packed = arr->get(2).UInt32();
  uint16_t weight_min = static_cast<uint16_t>(weight_packed >> 16);
  uint16_t weight_max = static_cast<uint16_t>(weight_packed & 0xFFFF);
  uint32_t stretch_packed = arr->get(3).UInt32();
  uint16_t stretch_min = static_cast<uint16_t>(stretch_packed >> 16);
  uint16_t stretch_max = static_cast<uint16_t>(stretch_packed & 0xFFFF);
  FontFaceStyle style;
  const auto& style_val = arr->get(4);
  if (!style_val.IsArray() || style_val.Array()->size() < 3) return nullptr;
  const auto& style_arr = style_val.Array();
  style.kind = static_cast<FontFaceStyleKind>(style_arr->get(0).Int32());
  style.oblique_angle_min = static_cast<int16_t>(style_arr->get(1).Int32());
  style.oblique_angle_max = static_cast<int16_t>(style_arr->get(2).Int32());

  std::vector<VariationAxis> variation_settings;
  {
    const auto& var_val = arr->get(5);
    if (var_val.IsArray()) {
      const auto& var_arr = var_val.Array();
      variation_settings.reserve(var_arr->size());
      for (size_t i = 0; i < var_arr->size(); ++i) {
        const auto& item = var_arr->get(i);
        if (!item.IsArray() || item.Array()->size() < 2) continue;
        VariationAxis axis;
        axis.tag = item.Array()->get(0).StdString();
        axis.value = static_cast<float>(item.Array()->get(1).Number());
        variation_settings.push_back(std::move(axis));
      }
    }
  }

  std::vector<UnicodeRange> unicode_range;
  {
    const auto& range_val = arr->get(6);
    if (range_val.IsArray()) {
      const auto& range_arr = range_val.Array();
      unicode_range.reserve(range_arr->size());
      for (size_t i = 0; i < range_arr->size(); ++i) {
        const auto& item = range_arr->get(i);
        if (!item.IsArray() || item.Array()->size() < 2) continue;
        UnicodeRange range;
        range.from = item.Array()->get(0).UInt32();
        range.to = item.Array()->get(1).UInt32();
        unicode_range.push_back(range);
      }
    }
  }

  return fml::AdoptRef(new FontFaceRule(
      std::move(family), std::move(sources), weight_min, weight_max,
      stretch_min, stretch_max, style, std::move(variation_settings),
      std::move(unicode_range)));
}

}  // namespace css
}  // namespace lynx
