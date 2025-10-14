// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_THIRD_PARTY_TXT_SRC_TXT_TYPEFACE_FONT_ASSET_PROVIDER_SKITY_H_
#define CLAY_THIRD_PARTY_TXT_SRC_TXT_TYPEFACE_FONT_ASSET_PROVIDER_SKITY_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "skity/text/font_manager.hpp"
#include "base/include/fml/macros.h"
#include "txt/font_asset_provider.h"

namespace txt {

class TypefaceFontStyleSet : public skity::FontStyleSet {
 public:
  TypefaceFontStyleSet();

  ~TypefaceFontStyleSet() override;

  void registerTypeface(std::shared_ptr<skity::Typeface> typeface);

  // |SkFontStyleSet|
  int Count() override;

  // |SkFontStyleSet|
  void GetStyle(int index, skity::FontStyle* style, std::string* name) override;

  // |SkFontStyleSet|
  std::shared_ptr<skity::Typeface> CreateTypeface(int index) override;

  // |SkFontStyleSet|
  std::shared_ptr<skity::Typeface> MatchStyle(const skity::FontStyle& pattern) override;

 private:
  std::vector<std::shared_ptr<skity::Typeface>> typefaces_;

  BASE_DISALLOW_COPY_AND_ASSIGN(TypefaceFontStyleSet);
};

class TypefaceFontAssetProvider : public FontAssetProvider {
 public:
  TypefaceFontAssetProvider();
  ~TypefaceFontAssetProvider() override;

  void RegisterTypeface(std::shared_ptr<skity::Typeface> typeface);

  void RegisterTypeface(std::shared_ptr<skity::Typeface> typeface,
                        std::string family_name_alias);

  // |FontAssetProvider|
  size_t GetFamilyCount() const override;

  // |FontAssetProvider|
  std::string GetFamilyName(int index) const override;

  // |FontAssetProvider|
  std::shared_ptr<skity::FontStyleSet> MatchFamily(const std::string& family_name) override;

 private:
  std::unordered_map<std::string, std::shared_ptr<TypefaceFontStyleSet>>
      registered_families_;
  std::vector<std::string> family_names_;

  BASE_DISALLOW_COPY_AND_ASSIGN(TypefaceFontAssetProvider);
};

}  // namespace txt

#endif  // CLAY_THIRD_PARTY_TXT_SRC_TXT_TYPEFACE_FONT_ASSET_PROVIDER_SKITY_H_
