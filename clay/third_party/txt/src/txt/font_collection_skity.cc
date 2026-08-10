// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "font_collection_skity.h"

#include <algorithm>
#include <cassert>
#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "txt/platform.h"
#include "txt/text_style.h"

namespace txt {

FontCollection::FamilyKey::FamilyKey(const std::vector<std::string>& families,
                                     const std::string& loc) {
  locale = loc;

  std::stringstream stream;
  for_each(families.begin(), families.end(),
           [&stream](const std::string& str) { stream << str << ','; });
  font_families = stream.str();
}

bool FontCollection::FamilyKey::operator==(
    const FontCollection::FamilyKey& other) const {
  return font_families == other.font_families && locale == other.locale;
}

size_t FontCollection::FamilyKey::Hasher::operator()(
    const FontCollection::FamilyKey& key) const {
  return std::hash<std::string>()(key.font_families) ^
         std::hash<std::string>()(key.locale);
}

FontCollection::FontCollection() : enable_font_fallback_(true) {}

FontCollection::~FontCollection() {}

size_t FontCollection::GetFontManagersCount() const {
  return GetFontManagerOrder().size();
}

void FontCollection::SetupDefaultFontManager(
    uint32_t font_initialization_data) {
  default_font_manager_ = GetDefaultFontManager(font_initialization_data);
}

void FontCollection::SetDefaultFontManager(
    std::shared_ptr<skity::FontManager> font_manager) {
  default_font_manager_ = std::move(font_manager);
}

void FontCollection::SetAssetFontManager(
    std::shared_ptr<skity::FontManager> font_manager) {
  asset_font_manager_ = std::move(font_manager);
}

// Return the available font managers in the order they should be queried.
std::vector<std::shared_ptr<skity::FontManager>>
FontCollection::GetFontManagerOrder() const {
  std::vector<std::shared_ptr<skity::FontManager>> order;
  if (default_font_manager_) {
    order.push_back(default_font_manager_);
  }
  if (asset_font_manager_) {
    order.push_back(asset_font_manager_);
  }
  return order;
}

void FontCollection::DisableFontFallback() {
  enable_font_fallback_ = false;
}

void FontCollection::ClearFontFamilyCache() {}

tttext::FontmgrCollection FontCollection::GetIFontCollection(
    const std::shared_ptr<DynamicFontManager>& dynamic_font_manager) {
  tttext::FontmgrCollection collection(nullptr);

  assert(default_font_manager_ != nullptr);
  if (default_font_manager_ != nullptr) {
#if OS_IOS
    collection.SetDefaultFontManager(
        std::make_shared<tttext::SkityFontManagerCoreText>());
#else
    collection.SetDefaultFontManager(
        std::make_shared<tttext::SkityFontManager>());
#endif
  }
  if (asset_font_manager_ != nullptr) {
#if OS_IOS
    collection.SetAssetFontManager(
        std::make_shared<tttext::SkityFontManagerCoreText>(
            asset_font_manager_));
#else
    collection.SetAssetFontManager(
        std::make_shared<tttext::SkityFontManager>(asset_font_manager_));
#endif
  }
  if (dynamic_font_manager != nullptr) {
#if OS_IOS
    collection.SetDynamicFontManager(
        std::make_shared<tttext::SkityFontManagerCoreText>(
            dynamic_font_manager));
#else
    collection.SetDynamicFontManager(
        std::make_shared<tttext::SkityFontManager>(dynamic_font_manager));
#endif
  }
  return collection;
}

FontCollection::VariationFontFamilies FontCollection::GetVariationFontFamilies(
    const std::vector<std::string>& font_families,
    const skity::FontStyle& font_style,
    const FontVariations& font_variations,
    const std::shared_ptr<DynamicFontManager>& dynamic_font_manager) {
  VariationFontFamilies result;
  result.font_families.reserve(std::max<size_t>(font_families.size(), 1));
  if (!dynamic_font_manager || font_variations.GetAxisValues().empty()) {
    result.font_families = font_families;
    return result;
  }

  const auto match_typeface = [&](const std::string& family) {
    const auto match = [&](const std::shared_ptr<skity::FontManager>& manager) {
      if (!manager) {
        return std::shared_ptr<skity::Typeface>();
      }
      if (family == tttext::FontmgrCollection::kDefaultFontFamily) {
        return manager->GetDefaultTypeface(font_style);
      }
      return manager->MatchFamilyStyle(family.c_str(), font_style);
    };

    auto typeface = match(asset_font_manager_);
    return typeface ? typeface : match(default_font_manager_);
  };

  const auto register_variation =
      [&](const std::shared_ptr<skity::Typeface>& base_typeface) {
        return dynamic_font_manager->GetOrCreateVariation(
            base_typeface, font_variations.GetAxisValues());
      };

  const auto set_primary_style = [&](const auto& variation) {
    if (!variation) {
      return;
    }
    result.font_style = variation->font_style;
    result.overrides_weight = variation->overrides_weight;
    result.overrides_width = variation->overrides_width;
    result.overrides_slant = variation->overrides_slant;
  };

  if (font_families.empty()) {
    if (default_font_manager_) {
      auto variation = register_variation(
          default_font_manager_->GetDefaultTypeface(font_style));
      set_primary_style(variation);
      if (variation) {
        result.font_families.emplace_back(variation->family_name);
      }
    }
    return result;
  }

  bool resolved_primary_typeface = false;
  for (const auto& family : font_families) {
    auto base_typeface = match_typeface(family);
    auto variation = register_variation(base_typeface);
    if (!resolved_primary_typeface && base_typeface) {
      resolved_primary_typeface = true;
      set_primary_style(variation);
    }
    if (variation) {
      result.font_families.emplace_back(variation->family_name);
    } else {
      result.font_families.emplace_back(family);
    }
  }
  return result;
}

}  // namespace txt
