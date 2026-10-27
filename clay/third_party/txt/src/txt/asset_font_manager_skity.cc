// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "txt/asset_font_manager_skity.h"

#include <utility>

#include "clay/fml/logging.h"
#include "skity/text/typeface.hpp"

namespace txt {

AssetFontManager::AssetFontManager(
    std::unique_ptr<FontAssetProvider> font_provider)
    : font_provider_(std::move(font_provider)) {
  FML_DCHECK(font_provider_ != nullptr);
}

AssetFontManager::~AssetFontManager() = default;

int AssetFontManager::OnCountFamilies() const {
  return font_provider_->GetFamilyCount();
}

std::string AssetFontManager::OnGetFamilyName(int index) const {
  return font_provider_->GetFamilyName(index);
}

std::shared_ptr<skity::FontStyleSet> AssetFontManager::OnCreateStyleSet(int index) const {
  FML_DCHECK(false);
  return nullptr;
}

std::shared_ptr<skity::FontStyleSet> AssetFontManager::OnMatchFamily(
    const char familyName[]) const {
  std::string family_name(familyName);
  return font_provider_->MatchFamily(family_name);
}

std::shared_ptr<skity::Typeface> AssetFontManager::OnMatchFamilyStyle(
    const char familyName[],
    const skity::FontStyle& style) const {
  auto font_style_set =
      font_provider_->MatchFamily(std::string(familyName));
  if (font_style_set == nullptr) {
    return nullptr;
  }
  return font_style_set->MatchStyle(style);
}

std::shared_ptr<skity::Typeface> AssetFontManager::OnMatchFamilyStyleCharacter(
    const char familyName[],
    const skity::FontStyle&,
    const char* bcp47[],
    int bcp47Count,
    skity::Unichar character) const {
  return nullptr;
}

std::shared_ptr<skity::Typeface> AssetFontManager::OnMakeFromData(
    std::shared_ptr<skity::Data> const&,
    int ttcIndex) const {
  FML_DCHECK(false);
  return nullptr;
}

std::shared_ptr<skity::Typeface> AssetFontManager::OnMakeFromFile(
    const char path[],
    int ttcIndex) const {
  FML_DCHECK(false);
  return nullptr;
}

std::shared_ptr<skity::Typeface> AssetFontManager::OnGetDefaultTypeface(
    skity::FontStyle const& font_style) const {
  return nullptr;
}

}  // namespace txt
