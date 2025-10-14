// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "font_collection_skity.h"

#include <algorithm>
#include <cassert>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "base/trace/native/trace_event.h"
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

tttext::FontmgrCollection FontCollection::GetIFontCollection() {
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
  return collection;
}

}  // namespace txt
