// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "txt/typeface_font_asset_provider_skity.h"

#include "clay/fml/logging.h"
#include "skity/text/typeface.hpp"

namespace txt {

TypefaceFontAssetProvider::TypefaceFontAssetProvider() = default;

TypefaceFontAssetProvider::~TypefaceFontAssetProvider() = default;

// |FontAssetProvider|
size_t TypefaceFontAssetProvider::GetFamilyCount() const {
  return family_names_.size();
}

// |FontAssetProvider|
std::string TypefaceFontAssetProvider::GetFamilyName(int index) const {
  return family_names_[index];
}

// |FontAssetProvider|
std::shared_ptr<skity::FontStyleSet> TypefaceFontAssetProvider::MatchFamily(
    const std::string& family_name) {
  auto found = registered_families_.find(CanonicalFamilyName(family_name));
  if (found == registered_families_.end()) {
    return nullptr;
  }
  return found->second;
}

void TypefaceFontAssetProvider::RegisterTypeface(std::shared_ptr<skity::Typeface> typeface) {
  if (typeface == nullptr) {
    return;
  }

  // TODO(jingle) There is no need for now
  std::string sk_family_name = "";
  // typeface->GetFamilyName(&sk_family_name);

  std::string family_name(sk_family_name.c_str(), sk_family_name.size());
  RegisterTypeface(std::move(typeface), std::move(family_name));
}

void TypefaceFontAssetProvider::RegisterTypeface(
    std::shared_ptr<skity::Typeface> typeface,
    std::string family_name_alias) {
  if (family_name_alias.empty()) {
    return;
  }

  std::string canonical_name = CanonicalFamilyName(family_name_alias);
  auto family_it = registered_families_.find(canonical_name);
  if (family_it == registered_families_.end()) {
    family_names_.push_back(family_name_alias);
    auto value = std::make_pair(canonical_name,
                                std::make_shared<TypefaceFontStyleSet>());
    family_it = registered_families_.emplace(std::move(value)).first;
  }
  family_it->second->registerTypeface(std::move(typeface));
}

TypefaceFontStyleSet::TypefaceFontStyleSet() = default;

TypefaceFontStyleSet::~TypefaceFontStyleSet() = default;

void TypefaceFontStyleSet::registerTypeface(std::shared_ptr<skity::Typeface> typeface) {
  if (typeface == nullptr) {
    return;
  }
  typefaces_.emplace_back(std::move(typeface));
}

int TypefaceFontStyleSet::Count() {
  return typefaces_.size();
}

void TypefaceFontStyleSet::GetStyle(int index,
                                    skity::FontStyle* style,
                                    std::string* name) {
  FML_DCHECK(static_cast<size_t>(index) < typefaces_.size());
  if (style) {
    *style = typefaces_[index]->GetFontStyle();
  }
  if (name) {
    name->clear();
  }
}

std::shared_ptr<skity::Typeface> TypefaceFontStyleSet::CreateTypeface(int i) {
  size_t index = i;
  if (index >= typefaces_.size()) {
    return nullptr;
  }
  return typefaces_[index];
}

std::shared_ptr<skity::Typeface> TypefaceFontStyleSet::MatchStyle(
    const skity::FontStyle& pattern) {
  return MatchStyleCSS3(pattern);
}

}  // namespace txt
