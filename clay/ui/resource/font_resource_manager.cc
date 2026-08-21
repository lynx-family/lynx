// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/resource/font_resource_manager.h"

#include <utility>

#include "base/include/fml/task_runner.h"
#include "base/include/string/string_utils.h"
#include "clay/fml/base64.h"
#include "clay/fml/logging.h"
#include "clay/net/loader/resource_loader_factory.h"
#include "clay/net/loader/resource_loader_intercept.h"
#include "clay/net/url/url_helper.h"
#include "clay/ui/common/isolate.h"

namespace clay {

FontResourceManager::FontResourceManager() = default;

FontResourceManager::~FontResourceManager() = default;

RawResource FontResourceManager::GetResource(const std::string& family_name) {
  auto iter = font_resource_map_.find(family_name);
  if (iter == font_resource_map_.end()) {
    return {0, nullptr};
  }

  return iter->second;
}

void FontResourceManager::DownloadFont(
    fml::RefPtr<fml::TaskRunner> load_task_runner,
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    std::shared_ptr<ServiceManager> service_manager, const std::string& url,
    const int url_index, const std::string& font_family) {
  if (url::ParseUriScheme(url) == url::UriSchemeType::kData) {
    fml::TaskRunner::RunNowOrPostTask(
        load_task_runner,
        [load_task_runner, intercept, service_manager, url, url_index,
         font_name = font_family, weak = weak_from_this()]() {
          auto self = weak.lock();
          if (!self) {
            return;
          }
          bool success = false;
          auto data = self->DecodeBase64Str(url);
          if (data.length > 0) {
            success = true;
          }
          self->OnDownloadEnd(success, url_index, font_name, data,
                              load_task_runner, intercept, service_manager);
        });
    return;
  }

  auto resource_loader = ResourceLoaderFactory::Create(
      url, load_task_runner, intercept, service_manager);
  if (!resource_loader) {
    FML_LOG(ERROR) << "create ResourceLoader fail";
    return;
  }
  resource_loader->Load(
      url,
      [load_task_runner, intercept, service_manager, weak = weak_from_this(),
       font_name = font_family,
       url_index](const uint8_t* font_stream, size_t len) {
        auto self = weak.lock();
        if (!self) {
          return;
        }

        bool success = (font_stream != nullptr && len > 0);
        RawResource data;
        if (success) {
          data = RawResource::MakeWithCopy(font_stream, len);
        } else {
          data = {0, nullptr};
        }

        self->OnDownloadEnd(success, url_index, font_name, data,
                            load_task_runner, intercept, service_manager);
      },
      ResourceType::kFont, true);
  font_loader_map_.emplace(font_family, std::move(resource_loader));
}

void FontResourceManager::OnDownloadEnd(
    bool success, const int url_index, const std::string& font_family,
    RawResource data, fml::RefPtr<fml::TaskRunner> load_task_runner,
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    std::shared_ptr<ServiceManager> service_manager) {
  auto iter = font_url_map_.find(font_family);
  if (iter == font_url_map_.end()) {
    FML_DLOG(ERROR) << "Unable to get font for family: " << font_family;
    return;
  }

  std::vector<std::string>& url_vec = iter->second;

  // fail
  if (!success) {
    // download by next url
    unsigned int index = url_index + 1;
    if (index >= url_vec.size()) {
      FML_DLOG(ERROR) << "Unable to get font for family: " << font_family;
      font_call_back_map_.erase(font_family);
      font_loader_map_.erase(font_family);
      return;
    } else {
      DownloadFont(load_task_runner, intercept, service_manager, url_vec[index],
                   index, font_family);
    }
    return;
  }

  // success , first : create font and cache
  font_resource_map_.emplace(font_family, data);

  // second : callback
  auto callback_iter = font_call_back_map_.find(font_family);
  if (callback_iter != font_call_back_map_.end()) {
    callback_iter->second(font_family, url_vec[url_index]);
  }
  font_call_back_map_.erase(font_family);
  font_loader_map_.erase(font_family);
}

bool FontResourceManager::HasFontResourceLoading(std::string font_family) {
  return font_loader_map_.find(font_family) != font_loader_map_.end();
}

void FontResourceManager::LoadFontAsync(
    fml::RefPtr<fml::TaskRunner> load_task_runner,
    std::shared_ptr<ResourceLoaderIntercept> intercept,
    std::shared_ptr<ServiceManager> service_manager,
    const std::string& font_family, std::vector<std::string> url_vec,
    const FontCallback& callback) {
  // first : find cache , avoid reloading
  if (font_url_map_.find(font_family) != font_url_map_.end()) {
    return;
  }

  // second : cache src
  if (url_vec.empty()) {
    return;
  }

  font_url_map_.emplace(font_family, std::move(url_vec));
  font_call_back_map_.emplace(font_family, callback);

  const int url_index = 0;
  const std::string& first_url = font_url_map_[font_family][url_index];

  // third : download by first url
  DownloadFont(load_task_runner, intercept, service_manager, first_url,
               url_index, font_family);
}

RawResource FontResourceManager::DecodeBase64Str(
    const std::string& base64_str) {
  if (!CheckFontScheme(base64_str.c_str())) {
    return {0, nullptr};
  }

  const std::string identify = "base64,";
  const int index = base64_str.find(identify);

  if (index == -1) {
    return {0, nullptr};
  }

  const int len = index + identify.length();
  const int data_len = base64_str.length() - len;
  const char* data = base64_str.c_str() + len;
  size_t encode_data_length;

  if (fml::Base64::Decode(data, data_len, nullptr, &encode_data_length) !=
      fml::Base64::kNoError) {
    return {0, nullptr};
  }

  auto encode_data = std::shared_ptr<uint8_t>(new uint8_t[encode_data_length],
                                              std::default_delete<uint8_t[]>());

  if (fml::Base64::Decode(data, data_len, encode_data.get(),
                          &encode_data_length) != fml::Base64::kNoError) {
    return {0, nullptr};
  }

  return {encode_data_length, encode_data};
}

void FontResourceManager::LoadFontSync(const std::string& font_family,
                                       std::vector<std::string>& url_vec) {
  // first : find cache
  if (font_url_map_.find(font_family) != font_url_map_.end()) {
    return;
  }

  // second : cache src
  if (url_vec.empty()) {
    return;
  }
  font_url_map_.emplace(font_family, std::move(url_vec));

  int index = -1;
  for (auto& one_url : font_url_map_[font_family]) {
    ++index;
    if (url::ParseUriScheme(one_url) == url::UriSchemeType::kData) {
      auto data = DecodeBase64Str(one_url);
      if (data.length > 0) {
        OnDownloadEnd(true, index, font_family, data, nullptr);
        break;
      }
      continue;
    }

    std::shared_ptr<ResourceLoader> loader =
        ResourceLoaderFactory::Create(one_url, nullptr);
    if (!loader) {
      FML_LOG(ERROR) << "create ResourceLoader fail";
      return;
    }
    auto data = loader->LoadSync(one_url, ResourceType::kFont);
    if (data.length > 0) {
      OnDownloadEnd(true, index, font_family, data, nullptr);
      break;
    }
  }

  return;
}

bool FontResourceManager::HasFontResource(const std::string& font_family) {
  if (font_resource_map_.find(font_family) != font_resource_map_.end()) {
    return true;
  }
  return false;
}

bool FontResourceManager::CheckFontScheme(const std::string_view base64_str) {
  // https://www.rfc-editor.org/rfc/rfc8081#ref-W3C.CR-css-fonts-3-20131003

  const char* scheme1 = "data:application/";
  const char* scheme2 = "data:font/";

  return lynx::base::BeginsWith(base64_str, scheme1) ||
         lynx::base::BeginsWith(base64_str, scheme2);
}

}  // namespace clay
