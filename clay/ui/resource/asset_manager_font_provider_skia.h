// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_ASSET_MANAGER_FONT_PROVIDER_SKIA_H_
#define CLAY_UI_RESOURCE_ASSET_MANAGER_FONT_PROVIDER_SKIA_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/macros.h"
#include "clay/third_party/txt/src/txt/font_asset_provider.h"
#include "clay/ui/resource/font_resource_manager.h"
#include "third_party/skia/include/core/SkFontMgr.h"
#include "third_party/skia/include/core/SkTypeface.h"

namespace clay {

class AssetManagerFontStyleSet : public SkFontStyleSet {
 public:
  AssetManagerFontStyleSet(
      const std::string& family_name,
      std::shared_ptr<FontResourceManager> font_resource_manager);

  ~AssetManagerFontStyleSet() override;

  void RegisterAsset(const std::string& file_path);

  // |SkFontStyleSet|
  int count() override;

  // |SkFontStyleSet|
  void getStyle(int index, SkFontStyle*, SkString* style) override;

  // |SkFontStyleSet|
  sk_sp<SkTypeface> createTypeface(int index) override;

  // |SkFontStyleSet|
  sk_sp<SkTypeface> matchStyle(const SkFontStyle& pattern) override;

 private:
  std::string family_name_;
  std::shared_ptr<FontResourceManager> font_resource_manager_;

  struct TypefaceAsset {
    explicit TypefaceAsset(const std::string& file_path);

    TypefaceAsset(const TypefaceAsset& other);

    ~TypefaceAsset();

    std::string path;
    sk_sp<SkTypeface> typeface;
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
  sk_sp<SkFontStyleSet> MatchFamily(const std::string& family_name) override;

 private:
  std::shared_ptr<FontResourceManager> font_resource_manager_;
  std::unordered_map<std::string, sk_sp<AssetManagerFontStyleSet>>
      registered_families_;
  std::vector<std::string> family_names_;

  BASE_DISALLOW_COPY_AND_ASSIGN(AssetManagerFontProvider);
};

}  // namespace clay

#endif  // CLAY_UI_RESOURCE_ASSET_MANAGER_FONT_PROVIDER_SKIA_H_
