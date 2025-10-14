
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RESOURCE_FONT_RESOURCE_MANAGER_H_
#define CLAY_UI_RESOURCE_FONT_RESOURCE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/net/loader/resource_loader.h"
#include "clay/net/resource_type.h"

namespace clay {

using FontCallback =
    std::function<void(const std::string& font_family, const std::string& url)>;
class ResourceLoaderIntercept;
class ServiceManager;

// pre load font and cache it util call TakeResource func
class FontResourceManager
    : public std::enable_shared_from_this<FontResourceManager> {
 public:
  FontResourceManager();
  ~FontResourceManager();

  RawResource GetResource(const std::string& font_family);

  void LoadFontAsync(fml::RefPtr<fml::TaskRunner> load_task_runner,
                     std::shared_ptr<ResourceLoaderIntercept> intercept,
                     std::shared_ptr<ServiceManager> service_manager,
                     const std::string& font_family,
                     std::vector<std::string> url_vec,
                     const FontCallback& callback);

  // call TakeResource to get resource
  void LoadFontSync(const std::string& font_family,
                    std::vector<std::string>& url_vec);

  bool HasFontResource(const std::string& font_family);

  void SetCallback(const FontCallback& callback);

  bool HasFontResourceLoading(std::string font_family);

 private:
  void OnDownloadEnd(
      bool success, const int url_index, const std::string& font_family,
      RawResource data, fml::RefPtr<fml::TaskRunner> load_task_runner,
      std::shared_ptr<ResourceLoaderIntercept> intercept = nullptr,
      std::shared_ptr<ServiceManager> service_manager = nullptr);
  void DownloadFont(fml::RefPtr<fml::TaskRunner> load_task_runner,
                    std::shared_ptr<ResourceLoaderIntercept> intercept,
                    std::shared_ptr<ServiceManager> service_manager,
                    const std::string& url, const int url_index,
                    const std::string& font_family);

  RawResource DecodeBase64Str(const std::string& base64_str);
  bool CheckFontScheme(const std::string_view base64_str);

  std::map<std::string, std::vector<std::string>> font_url_map_;
  std::map<std::string, std::shared_ptr<ResourceLoader>> font_loader_map_;
  std::map<std::string, RawResource> font_resource_map_;
  std::map<std::string, FontCallback> font_call_back_map_;
};

}  // namespace clay

#endif  // CLAY_UI_RESOURCE_FONT_RESOURCE_MANAGER_H_
