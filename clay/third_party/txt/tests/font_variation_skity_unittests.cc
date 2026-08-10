// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "build/build_config.h"
#include "clay/testing/testing.h"
#include "gtest/gtest.h"
#if defined(OS_IOS) || defined(OS_MAC) || defined(OS_OSX)
#include "skity/text/ports/typeface_ct.hpp"
#endif
#include "skity/text/typeface.hpp"
#include "txt/asset_font_manager_skity.h"
#include "txt/font_collection_skity.h"
#include "txt/font_features.h"
#include "txt/typeface_font_asset_provider_skity.h"

namespace txt {
namespace {

constexpr char kWeightWidthFont[] = "Variable.ttf";
constexpr char kOpticalSizeFont[] =
    "ab40c89624a6104e5d0a2308e448a989302f515b.ttf";
constexpr char kSlantFont[] = "TestGVAR-Composite-0.ttf";

skity::FourByteTag Tag(const char (&axis)[5]) {
  return skity::SetFourByteTag(axis[0], axis[1], axis[2], axis[3]);
}

std::shared_ptr<skity::Typeface> LoadFixture(const char* name) {
  const std::string path =
      std::string(clay::testing::GetFixturesPath()) + "/" + name;
  return skity::Typeface::MakeFromFile(path.c_str());
}

std::optional<float> GetMaterializedAxisValue(
    const std::shared_ptr<skity::Typeface>& typeface,
    skity::FourByteTag axis) {
#if defined(OS_IOS) || defined(OS_MAC) || defined(OS_OSX)
  // CoreText retains the exact design coordinates even when Skity's Darwin
  // GetVariationDesignPosition() adapter reports an empty position.
  if (!typeface) {
    return std::nullopt;
  }
  CTFontRef font = skity::TypefaceCT::CTFontFromTypeface(typeface);
  CFDictionaryRef variation = CTFontCopyVariation(font);
  if (!variation) {
    return std::nullopt;
  }
  const int64_t tag = axis;
  CFNumberRef tag_ref =
      CFNumberCreate(kCFAllocatorDefault, kCFNumberLongLongType, &tag);
  CFTypeRef value_ref =
      tag_ref ? CFDictionaryGetValue(variation, tag_ref) : nullptr;
  float value = 0.f;
  const bool found = value_ref &&
                     CFGetTypeID(value_ref) == CFNumberGetTypeID() &&
                     CFNumberGetValue(static_cast<CFNumberRef>(value_ref),
                                      kCFNumberFloatType, &value);
  if (tag_ref) {
    CFRelease(tag_ref);
  }
  CFRelease(variation);
  return found ? std::optional<float>(value) : std::nullopt;
#else
  if (!typeface) {
    return std::nullopt;
  }
  for (const auto& coordinate :
       typeface->GetVariationDesignPosition().GetCoordinates()) {
    if (coordinate.axis == axis) {
      return coordinate.value;
    }
  }
  return std::nullopt;
#endif
}

class DefaultTypefaceFontManager final : public AssetFontManager {
 public:
  explicit DefaultTypefaceFontManager(
      std::shared_ptr<skity::Typeface> default_typeface)
      : AssetFontManager(std::make_unique<TypefaceFontAssetProvider>()),
        default_typeface_(std::move(default_typeface)) {}

  int default_match_count() const { return default_match_count_; }

 private:
  std::shared_ptr<skity::Typeface> OnGetDefaultTypeface(
      const skity::FontStyle&) const override {
    ++default_match_count_;
    return default_typeface_;
  }

  std::shared_ptr<skity::Typeface> default_typeface_;
  mutable int default_match_count_ = 0;
};

TEST(VariableFontSkityTest, MaterializesFiltersAndCachesRealAxes) {
  auto weight_width_base = LoadFixture(kWeightWidthFont);
  ASSERT_NE(weight_width_base, nullptr);

  auto manager = std::make_shared<DynamicFontManager>();
  const std::map<std::string, float> requested_axes = {
      {"opsz", 42.f}, {"slnt", -10.f}, {"wdth", 150.f}, {"wght", 715.f}};
  auto first = manager->GetOrCreateVariation(weight_width_base, requested_axes);
  ASSERT_TRUE(first.has_value());
  EXPECT_TRUE(first->overrides_weight);
  EXPECT_TRUE(first->overrides_width);
  EXPECT_FALSE(first->overrides_slant);

  auto varied =
      manager->MatchFamilyStyle(first->family_name.c_str(), first->font_style);
  ASSERT_NE(varied, nullptr);
  EXPECT_NE(varied->TypefaceId(), weight_width_base->TypefaceId());
  ASSERT_TRUE(GetMaterializedAxisValue(varied, Tag("wght")).has_value());
  EXPECT_FLOAT_EQ(*GetMaterializedAxisValue(varied, Tag("wght")), 715.f);
  ASSERT_TRUE(GetMaterializedAxisValue(varied, Tag("wdth")).has_value());
  EXPECT_FLOAT_EQ(*GetMaterializedAxisValue(varied, Tag("wdth")), 150.f);
  EXPECT_FALSE(GetMaterializedAxisValue(varied, Tag("opsz")).has_value());
  EXPECT_FALSE(GetMaterializedAxisValue(varied, Tag("slnt")).has_value());

  auto cached =
      manager->GetOrCreateVariation(weight_width_base, requested_axes);
  ASSERT_TRUE(cached.has_value());
  EXPECT_EQ(cached->family_name, first->family_name);
  auto cached_typeface = manager->MatchFamilyStyle(cached->family_name.c_str(),
                                                   cached->font_style);
  ASSERT_NE(cached_typeface, nullptr);
  EXPECT_EQ(cached_typeface->TypefaceId(), varied->TypefaceId());

  auto optical_size_base = LoadFixture(kOpticalSizeFont);
  ASSERT_NE(optical_size_base, nullptr);
  auto optical_size =
      manager->GetOrCreateVariation(optical_size_base, {{"opsz", 42.f}});
  ASSERT_TRUE(optical_size.has_value());
  auto optical_size_typeface = manager->MatchFamilyStyle(
      optical_size->family_name.c_str(), optical_size->font_style);
  ASSERT_NE(optical_size_typeface, nullptr);
  ASSERT_TRUE(
      GetMaterializedAxisValue(optical_size_typeface, Tag("opsz")).has_value());
  EXPECT_FLOAT_EQ(*GetMaterializedAxisValue(optical_size_typeface, Tag("opsz")),
                  42.f);

  auto slant_base = LoadFixture(kSlantFont);
  ASSERT_NE(slant_base, nullptr);
  auto slant = manager->GetOrCreateVariation(slant_base, {{"slnt", -10.f}});
  ASSERT_TRUE(slant.has_value());
  EXPECT_TRUE(slant->overrides_slant);
  EXPECT_FALSE(slant->overrides_weight);
  EXPECT_FALSE(slant->overrides_width);
  auto slant_typeface =
      manager->MatchFamilyStyle(slant->family_name.c_str(), slant->font_style);
  ASSERT_NE(slant_typeface, nullptr);
  ASSERT_TRUE(
      GetMaterializedAxisValue(slant_typeface, Tag("slnt")).has_value());
  EXPECT_FLOAT_EQ(*GetMaterializedAxisValue(slant_typeface, Tag("slnt")),
                  -10.f);
}

TEST(VariableFontSkityTest, ResolvesSansSerifThroughDefaultTypeface) {
  auto default_typeface = LoadFixture(kWeightWidthFont);
  ASSERT_NE(default_typeface, nullptr);

  auto default_manager =
      std::make_shared<DefaultTypefaceFontManager>(default_typeface);
  FontCollection collection;
  collection.SetDefaultFontManager(default_manager);

  FontVariations variations;
  variations.SetAxisValue("wdth", 150.f);
  variations.SetAxisValue("wght", 715.f);
  auto dynamic_manager = std::make_shared<DynamicFontManager>();
  auto result = collection.GetVariationFontFamilies(
      {tttext::FontmgrCollection::kDefaultFontFamily},
      skity::FontStyle::Normal(), variations, dynamic_manager);

  EXPECT_EQ(default_manager->default_match_count(), 1);
  ASSERT_EQ(result.font_families.size(), 1u);
  EXPECT_NE(result.font_families.front().find("__clay_internal_variable_font_"),
            std::string::npos);
  ASSERT_TRUE(result.font_style.has_value());
  EXPECT_TRUE(result.overrides_weight);
  EXPECT_TRUE(result.overrides_width);

  auto varied = dynamic_manager->MatchFamilyStyle(
      result.font_families.front().c_str(), *result.font_style);
  ASSERT_NE(varied, nullptr);
  ASSERT_TRUE(GetMaterializedAxisValue(varied, Tag("wght")).has_value());
  EXPECT_FLOAT_EQ(*GetMaterializedAxisValue(varied, Tag("wght")), 715.f);
  ASSERT_TRUE(GetMaterializedAxisValue(varied, Tag("wdth")).has_value());
  EXPECT_FLOAT_EQ(*GetMaterializedAxisValue(varied, Tag("wdth")), 150.f);
}

}  // namespace
}  // namespace txt

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
