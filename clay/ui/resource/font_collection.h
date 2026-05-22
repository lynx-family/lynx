// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_FONT_COLLECTION_H_
#define CLAY_UI_RESOURCE_FONT_COLLECTION_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/ui/resource/asset_font_manager_clay.h"
#include "clay/ui/resource/font_resource_manager.h"
#include "clay/ui/ui_rendering_backend.h"

namespace clay {

using FontDownloadCallback = std::function<void()>;

class ResourceLoaderIntercept;
class ServiceManager;

// FontCollection is the collection of font for Clay. It is a singleton which is
// also can be accessed by `clay::Isolate`.
//
// IMPORTANT: For reuse of resource, the singleton would not clear resource
// actively. In other words, it will keep all resource until called by
// `PageView::NotifyLowMemory()` or the process termination.
// FontCollection is singleton now, we could not use fml::WeakPtr here because
// it is not thread safe in destruction when the process termination.
class FontCollection : public std::enable_shared_from_this<FontCollection> {
 public:
  static std::shared_ptr<FontCollection> Instance();

  ~FontCollection();

  std::shared_ptr<txt::FontCollection> GetFontCollection() const;

  void SetupDefaultFontManager(uint32_t font_initialization_data);

  void PreLoadFontOnMem(fml::RefPtr<fml::TaskRunner> load_task_runner,
                        std::shared_ptr<ResourceLoaderIntercept> intercept,
                        std::shared_ptr<ServiceManager> service_manager,
                        const std::string& font_family,
                        std::vector<std::string> urls);

  void RegisterCallback(const std::string& font_family,
                        const FontDownloadCallback& callback);

  bool HasFontResource(const std::string& font_family);

  bool HasFontResourceLoading(const std::string& font_family);

  void OnLoadFontEnd(const std::string& font_family);

  void RegisterAssetFont(const std::string& family_name,
                         const std::string& asset_path);

  void ClearFontFamilyCache();

  bool IfSystemFontFamily(std::string& font_family);

#ifndef ENABLE_SKITY
  sk_sp<AssetFontManagerClay> asset_font_manager() const {
    return asset_font_manager_;
  }
#else
  std::shared_ptr<AssetFontManagerClay> asset_font_manager() const {
    return asset_font_manager_;
  }
#endif  // ENABLE_SKITY

 private:
  std::shared_ptr<txt::FontCollection> collection_;
  std::shared_ptr<FontResourceManager> font_resource_manager_;
#ifndef ENABLE_SKITY
  sk_sp<AssetFontManagerClay> asset_font_manager_;
#else
  std::shared_ptr<AssetFontManagerClay> asset_font_manager_;
#endif  // ENABLE_SKITY
  mutable std::atomic<int32_t> asset_font_manager_ref_;

  FontCollection();

  std::unordered_multimap<std::string, FontDownloadCallback>
      font_download_callback_;

  FRIEND_TEST(FontResourceManagerTest, GetLocalResourceTest);
  FRIEND_TEST(FontResourceManagerTest, DISABLED_GetNetWorkResourceTest);
  FRIEND_TEST(FontResourceManagerTest, FontCollectionTest);

  BASE_DISALLOW_COPY_AND_ASSIGN(FontCollection);
};
}  // namespace clay

#endif  // CLAY_UI_RESOURCE_FONT_COLLECTION_H_
