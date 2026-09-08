// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/resource/font_collection.h"

#include <string>

#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/third_party/txt/src/txt/platform.h"
#include "clay/ui/common/isolate.h"

namespace clay {

std::shared_ptr<FontCollection> FontCollection::Instance() {
  static std::shared_ptr<FontCollection> instance = nullptr;
  if (!instance) {
    instance = std::shared_ptr<FontCollection>(new FontCollection());
  }
  return instance;
}

FontCollection::FontCollection()
    : collection_(std::make_shared<txt::FontCollection>()) {
  font_resource_manager_ = std::make_shared<FontResourceManager>();
#ifndef ENABLE_SKITY
  asset_font_manager_ =
      sk_make_sp<AssetFontManagerClay>(font_resource_manager_);
#else
  asset_font_manager_ =
      std::make_shared<AssetFontManagerClay>(font_resource_manager_);
#endif  // ENABLE_SKITY
  collection_->SetAssetFontManager(asset_font_manager_);
}

FontCollection::~FontCollection() {
  collection_.reset();
  asset_font_manager_.reset();
  font_resource_manager_.reset();
#ifndef ENABLE_SKITY
  SkGraphics::PurgeFontCache();
#endif  // ENABLE_SKITY
}

std::shared_ptr<txt::FontCollection> FontCollection::GetFontCollection() const {
  return collection_;
}

void FontCollection::SetupDefaultFontManager(
    uint32_t font_initialization_data) {
  collection_->SetupDefaultFontManager(font_initialization_data);
}

void FontCollection::RegisterAssetFont(const std::string& family_name,
                                       const std::string& asset_path) {
  if (!asset_font_manager_) {
    FML_LOG(ERROR) << "asset_font_manager should not be null";
    FML_DCHECK(false);
    return;
  }
  AssetManagerFontProvider& font_provider =
      asset_font_manager_->font_provider();

  font_provider.RegisterAsset(family_name, asset_path);
}

void FontCollection::PreLoadFontOnMem(
    fml::RefPtr<fml::TaskRunner> load_task_runner,
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    std::shared_ptr<ServiceManager> service_manager,
    const std::string& font_family, std::vector<std::string> urls) {
  if (!font_resource_manager_) {
    FML_LOG(ERROR) << "font_resource_manager should not be null";
    FML_DCHECK(false);
    return;
  }

  FontCallback callback = [weak = weak_from_this()](
                              const std::string& font_family,
                              const std::string& url) {
    // first : register font
    // One font family may specify multiple fonts, supporting matching the most
    // appropriate font to show, now we only use one font
    auto self = weak.lock();
    if (!self) {
      return;
    }
    const std::string str = "not use";
    self->RegisterAssetFont(font_family, str);

    self->OnLoadFontEnd(font_family);
  };
  font_resource_manager_->LoadFontAsync(load_task_runner, intercept,
                                        service_manager, font_family,
                                        std::move(urls), callback);
}

void FontCollection::RegisterCallback(const std::string& font_family,
                                      const FontDownloadCallback& callback) {
  if (HasFontResourceLoading(font_family)) {
    font_download_callback_.emplace(font_family, callback);
  }
}

bool FontCollection::HasFontResource(const std::string& font_family) {
  if (!font_resource_manager_) {
    FML_LOG(ERROR) << "font_resource_manager should not be null";
    FML_DCHECK(false);
    return false;
  }

  return font_resource_manager_->HasFontResource(font_family);
}

bool FontCollection::HasFontResourceLoading(const std::string& font_family) {
  if (!font_resource_manager_) {
    FML_LOG(ERROR) << "font_resource_manager should not be null";
    FML_DCHECK(false);
    return false;
  }

  return font_resource_manager_->HasFontResourceLoading(font_family);
}

void FontCollection::ClearFontFamilyCache() {
  collection_->ClearFontFamilyCache();
}

void FontCollection::OnLoadFontEnd(const std::string& font_family) {
  auto range = font_download_callback_.equal_range(font_family);
  collection_->ClearFontFamilyCache();

  for (auto it = range.first; it != range.second; ++it) {
    it->second();
  }

  font_download_callback_.erase(font_family);
}

bool FontCollection::IfSystemFontFamily(std::string& font_family) {
  auto default_font_manager = txt::GetDefaultFontManager(0);
  auto font_style_set =
      FONT_MANAGER_MATCH_FAMILY(default_font_manager, font_family.c_str());
  return FONT_STYLE_SET_COUNT(font_style_set) != 0;
}

}  // namespace clay
