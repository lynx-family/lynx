// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_THIRD_PARTY_TXT_SRC_TXT_FONT_COLLECTION_SKITY_H_
#define CLAY_THIRD_PARTY_TXT_SRC_TXT_FONT_COLLECTION_SKITY_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/macros.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck
#include "tttext/tttext_headers.h"
#include "txt/asset_font_manager_skity.h"
#include "txt/text_style.h"

namespace txt {

// Macro Notice: enable_skity implies enable enable_tttext
class FontCollection : public std::enable_shared_from_this<FontCollection> {
 public:
  FontCollection();

  ~FontCollection();

  size_t GetFontManagersCount() const;

  void SetupDefaultFontManager(uint32_t font_initialization_data);
  void SetDefaultFontManager(std::shared_ptr<skity::FontManager> font_manager);
  void SetAssetFontManager(std::shared_ptr<skity::FontManager> font_manager);
  // void SetDynamicFontManager(sk_sp<SkFontMgr> font_manager);
  // void SetTestFontManager(sk_sp<SkFontMgr> font_manager);

  // Do not provide alternative fonts that can match characters which are
  // missing from the requested font family.
  void DisableFontFallback();

  // Remove all entries in the font family cache.
  void ClearFontFamilyCache();

  // Construct a Skia text layout FontCollection based on this collection.
  tttext::FontmgrCollection GetIFontCollection();

 private:
  struct FamilyKey {
    FamilyKey(const std::vector<std::string>& families, const std::string& loc);

    // Concatenated string with all font families.
    std::string font_families;
    std::string locale;

    bool operator==(const FamilyKey& other) const;

    struct Hasher {
      size_t operator()(const FamilyKey& key) const;
    };
  };

  std::shared_ptr<skity::FontManager> default_font_manager_;
  std::shared_ptr<skity::FontManager> asset_font_manager_;
  // sk_sp<SkFontMgr> dynamic_font_manager_;
  // sk_sp<SkFontMgr> test_font_manager_;

  std::unordered_map<std::string, std::vector<std::string>>
      fallback_fonts_for_locale_;
  bool enable_font_fallback_;

  std::vector<std::shared_ptr<skity::FontManager>> GetFontManagerOrder() const;
  // Sorts in-place a group of SkTypeface from an SkTypefaceSet into a
  // reasonable order for future queries.
  FRIEND_TEST(FontCollectionTest, CheckSkTypefacesSorting);

  BASE_DISALLOW_COPY_AND_ASSIGN(FontCollection);
};

}  // namespace txt

#endif  // CLAY_THIRD_PARTY_TXT_SRC_TXT_FONT_COLLECTION_SKITY_H_
