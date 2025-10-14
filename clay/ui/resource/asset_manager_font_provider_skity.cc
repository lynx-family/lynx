// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/resource/asset_manager_font_provider_skity.h"

#include <fstream>
#include <utility>

#include "base/include/fml/macros.h"
#include "clay/fml/logging.h"
#include "clay/net/resource_type.h"

namespace clay {
AssetManagerFontProvider::AssetManagerFontProvider(
    std::shared_ptr<FontResourceManager> font_resource_manager)
    : font_resource_manager_(font_resource_manager) {}

AssetManagerFontProvider::~AssetManagerFontProvider() = default;

// |FontAssetProvider|
size_t AssetManagerFontProvider::GetFamilyCount() const {
  return family_names_.size();
}

// |FontAssetProvider|
std::string AssetManagerFontProvider::GetFamilyName(int index) const {
  FML_DCHECK(index >= 0 && static_cast<size_t>(index) < family_names_.size());
  return family_names_[index];
}

// |FontAssetProvider|
std::shared_ptr<skity::FontStyleSet> AssetManagerFontProvider::MatchFamily(
    const std::string& family_name) {
  auto found = registered_families_.find(CanonicalFamilyName(family_name));
  if (found == registered_families_.end()) {
    return nullptr;
  }
  return found->second;
}

void AssetManagerFontProvider::RegisterAsset(const std::string& family_name,
                                             const std::string& file_path) {
  std::string canonical_name = CanonicalFamilyName(family_name);
  auto family_it = registered_families_.find(canonical_name);

  if (family_it == registered_families_.end()) {
    family_names_.push_back(family_name);
    auto value = std::make_pair(canonical_name,
                                std::make_shared<AssetManagerFontStyleSet>(
                                    family_name, font_resource_manager_));
    family_it = registered_families_.emplace(std::move(value)).first;
  }

  family_it->second->RegisterAsset(file_path);
}

AssetManagerFontStyleSet::AssetManagerFontStyleSet(
    const std::string& family_name,
    std::shared_ptr<FontResourceManager> font_resource_manager)
    : family_name_(family_name),
      font_resource_manager_(font_resource_manager) {}

AssetManagerFontStyleSet::~AssetManagerFontStyleSet() = default;

void AssetManagerFontStyleSet::RegisterAsset(const std::string& asset) {
  assets_.emplace_back(asset);
}

int AssetManagerFontStyleSet::Count() { return assets_.size(); }

void AssetManagerFontStyleSet::GetStyle(int index, skity::FontStyle* style,
                                        std::string* name) {
  FML_DCHECK(index < static_cast<int>(assets_.size()));
  if (style) {
    auto typeface = CreateTypeface(index);
    if (typeface) {
      *style = typeface->GetFontStyle();
    }
  }
  if (name) {
    *name = family_name_.c_str();
  }
}

std::shared_ptr<skity::Typeface> AssetManagerFontStyleSet::CreateTypeface(
    int i) {
  size_t index = i;
  if (index >= assets_.size()) {
    return nullptr;
  }

  TypefaceAsset& asset = assets_[index];
  if (!asset.typeface) {
    // TODO(jiangwenlong) : now we only support one font file ,support more font
    // files
    RawResource resource = font_resource_manager_->GetResource(family_name_);
    if (resource.data == nullptr) {
      return nullptr;
    }

    std::shared_ptr<skity::Data> data =
        skity::Data::MakeWithCopy(resource.data.get(), resource.length);
    auto typeface = skity::Typeface::MakeFromData(data);
    asset.typeface = std::move(typeface);
    if (!asset.typeface) {
      FML_DLOG(ERROR) << "Unable to load font asset for family: "
                      << family_name_;
      return nullptr;
    }
  }

  return asset.typeface;
}

std::shared_ptr<skity::Typeface> AssetManagerFontStyleSet::MatchStyle(
    const skity::FontStyle& pattern) {
  return MatchStyleCSS3(pattern);
}

AssetManagerFontStyleSet::TypefaceAsset::TypefaceAsset(
    const std::string& file_path)
    : path(file_path) {}

AssetManagerFontStyleSet::TypefaceAsset::TypefaceAsset(
    const AssetManagerFontStyleSet::TypefaceAsset& other) = default;

AssetManagerFontStyleSet::TypefaceAsset::~TypefaceAsset() = default;
}  // namespace clay
