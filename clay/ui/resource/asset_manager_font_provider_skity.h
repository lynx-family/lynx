// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_ASSET_MANAGER_FONT_PROVIDER_SKITY_H_
#define CLAY_UI_RESOURCE_ASSET_MANAGER_FONT_PROVIDER_SKITY_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/macros.h"
#include "clay/third_party/txt/src/txt/font_asset_provider.h"
#include "clay/ui/resource/font_resource_manager.h"
#include "skity/text/font_manager.hpp"

namespace clay {

class AssetManagerFontStyleSet : public skity::FontStyleSet {
 public:
  AssetManagerFontStyleSet(
      const std::string& family_name,
      std::shared_ptr<FontResourceManager> font_resource_manager);

  ~AssetManagerFontStyleSet() override;

  void RegisterAsset(const std::string& file_path);

  int Count() override;

  void GetStyle(int index, skity::FontStyle* style, std::string* name) override;

  std::shared_ptr<skity::Typeface> CreateTypeface(int index) override;

  std::shared_ptr<skity::Typeface> MatchStyle(
      const skity::FontStyle& pattern) override;

 private:
  std::string family_name_;
  std::shared_ptr<FontResourceManager> font_resource_manager_;

  struct TypefaceAsset {
    explicit TypefaceAsset(const std::string& file_path);

    TypefaceAsset(const TypefaceAsset& other);

    ~TypefaceAsset();

    std::string path;
    std::shared_ptr<skity::Typeface> typeface = nullptr;
  };
  std::vector<TypefaceAsset> assets_;

  BASE_DISALLOW_COPY_AND_ASSIGN(AssetManagerFontStyleSet);
};

class AssetManagerFontProvider : public txt::FontAssetProvider {
 public:
  explicit AssetManagerFontProvider(
      std::shared_ptr<FontResourceManager> font_resource_manager);
  ~AssetManagerFontProvider() override;

  void RegisterAsset(const std::string& family_name,
                     const std::string& file_path);

  // |FontAssetProvider|
  size_t GetFamilyCount() const override;

  // |FontAssetProvider|
  std::string GetFamilyName(int index) const override;

  // |FontAssetProvider|
  std::shared_ptr<skity::FontStyleSet> MatchFamily(
      const std::string& family_name) override;

 private:
  std::shared_ptr<FontResourceManager> font_resource_manager_;
  std::unordered_map<std::string, std::shared_ptr<AssetManagerFontStyleSet>>
      registered_families_;
  std::vector<std::string> family_names_;

  BASE_DISALLOW_COPY_AND_ASSIGN(AssetManagerFontProvider);
};

}  // namespace clay

#endif  // CLAY_UI_RESOURCE_ASSET_MANAGER_FONT_PROVIDER_SKITY_H_
