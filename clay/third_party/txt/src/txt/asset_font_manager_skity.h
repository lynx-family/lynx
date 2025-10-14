// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_THIRD_PARTY_TXT_SRC_TXT_ASSET_FONT_MANAGER_SKITY_H_
#define CLAY_THIRD_PARTY_TXT_SRC_TXT_ASSET_FONT_MANAGER_SKITY_H_

#include <memory>
#include <string>

#include "skity/text/font_manager.hpp"
#include "txt/typeface_font_asset_provider_skity.h"

namespace txt {

class AssetFontManager : public skity::FontManager {
 public:
  explicit AssetFontManager(std::unique_ptr<FontAssetProvider> font_provider);

  ~AssetFontManager() override;

 protected:
  std::shared_ptr<skity::FontStyleSet> OnMatchFamily(const char familyName[]) const override;

  std::unique_ptr<FontAssetProvider> font_provider_;

 private:
  int OnCountFamilies() const override;

  std::string OnGetFamilyName(int index) const override;

  std::shared_ptr<skity::FontStyleSet> OnCreateStyleSet(int index) const override;

  std::shared_ptr<skity::Typeface> OnMatchFamilyStyle(const char familyName[],
                                      const skity::FontStyle&) const override;

  std::shared_ptr<skity::Typeface> OnMatchFamilyStyleCharacter(
      const char familyName[],
      const skity::FontStyle&,
      const char* bcp47[],
      int bcp47Count,
      skity::Unichar character) const override;

  std::shared_ptr<skity::Typeface> OnMakeFromData(
      std::shared_ptr<skity::Data> const&,
      int ttcIndex) const override;

  std::shared_ptr<skity::Typeface> OnMakeFromFile(const char path[],
                                                  int ttcIndex) const override;

  std::shared_ptr<skity::Typeface> OnGetDefaultTypeface(
      skity::FontStyle const& font_style) const override;

  BASE_DISALLOW_COPY_AND_ASSIGN(AssetFontManager);
};

}  // namespace txt

#endif  // CLAY_THIRD_PARTY_TXT_SRC_TXT_ASSET_FONT_MANAGER_SKITY_H_
