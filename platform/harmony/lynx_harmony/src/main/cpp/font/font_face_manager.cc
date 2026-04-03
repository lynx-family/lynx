// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/font/font_face_manager.h"

#include <memory>

#include "base/include/datauri_utils.h"
#include "base/include/fml/make_copyable.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/font/system_font_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {

void FontFace::ParseAndAddSrc(const std::string& src) {
  static constexpr const char* kUrlIdent = "url(";
  static constexpr const char* kLocalIdent = "local(";
  static constexpr const char* kRightParenthesis = ")";
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FONT_FACE_PARSE_AND_ADD_SRC);
  auto ret = base::SplitStringIgnoreBracket(src, ',');
  size_t indent_length = 0;
  for (auto& item : ret) {
    std::size_t start_pos = item.find(kUrlIdent);
    Type type;
    if (start_pos != std::string::npos) {
      type = Type::URL;
    } else if ((start_pos = item.find(kLocalIdent)) != std::string::npos) {
      type = Type::LOCAL;
    } else {
      continue;
    }

    std::size_t end_pos = item.find(kRightParenthesis);
    if (end_pos != std::string::npos) {
      indent_length =
          type == Type::URL ? std::strlen(kUrlIdent) : std::strlen(kLocalIdent);
      start_pos = start_pos + indent_length;
      char current = item[start_pos];
      // trim "url('" or 'url("'
      while (start_pos < end_pos) {
        current = item[start_pos];
        if (current != ' ' && current != '\'' && current != '"') {
          break;
        }
        start_pos++;
      }

      // trim "')" or '")'
      end_pos--;
      while (end_pos > start_pos) {
        current = item[end_pos];
        if (current != ' ' && current != '\'' && current != '"') {
          break;
        }
        end_pos--;
      }

      std::string url = item.substr(start_pos, end_pos - start_pos + 1);
      // if enable global font collection,we need to use font_family+ur.hash to
      // make a unique key!
      std::string unique_key =
          LynxEnv::GetInstance().EnableGlobalFontCollection()
              ? (font_family_ + std::to_string(std::hash<std::string>{}(url)))
              : font_family_;
      src_data_.emplace_back(
          FontSrcData{type, std::move(url), std::move(unique_key)});
    }
  }
}

void FontFaceManager::LoadFontWithUrl(int sign, const std::string& font_family,
                                      const std::string& src,
                                      const FontFace::Type type,
                                      FontResourceCallback callback) {
  auto font_callback = std::move(callback);

  if (type == FontFace::Type::LOCAL) {
    // TODO(linxs:) support it later
    std::vector<uint8_t> empty_data;
    callback(font_family, -1, empty_data, 0);
    GetFontCollection()->SetLoadingFontState(
        font_family, FontCollectionHarmony::FontLoadingState::kLoadError);
  } else if (type == FontFace::Type::URL) {
    const auto& font_face_cache = GetFontFaceCache();
    std::vector<uint8_t> cached_data;
    bool is_data_uri = base::DataURIUtil::IsDataURI(src);
    if (!is_data_uri &&
        (!node_owner_ || font_face_cache.GetFontCache(src, cached_data))) {
      font_callback(font_family, 0, cached_data, cached_data.size());
      return;
    }

    if (is_data_uri) {
      // FIXME(linxs): to be determined later, is it necessary to post decode
      // to async thread?
      std::vector<uint8_t> font_data;
      size_t decoded_bas64_length =
          base::DataURIUtil::DecodeDataURI(src, [&font_data](size_t size) {
            font_data.resize(size);
            return reinterpret_cast<char*>(font_data.data());
          });
      font_callback(font_family, 0, font_data, decoded_bas64_length);
      return;
    }

    // http url
    if (GetFontCollection()->GetFontLoadingState(font_family) !=
        FontCollectionHarmony::FontLoadingState::kUndefined) {
      font_family_pending_shadow_node_[font_family].emplace_back(sign);
      return;
    }

    GetFontCollection()->SetLoadingFontState(
        font_family, FontCollectionHarmony::FontLoadingState::kLoading);
    auto& resource_loader = node_owner_->Context()->GetResourceLoader();
    auto request = pub::LynxResourceRequest{src, pub::LynxResourceType::kFont};
    resource_loader->LoadResource(
        request,
        [sign, font_family, src, font_callback = std::move(font_callback),
         weak_self = std::weak_ptr<FontFaceManager>(shared_from_this())](
            pub::LynxResourceResponse& response) mutable {
          auto shared_self = weak_self.lock();

          if (!shared_self) {
            return;
          }

          auto task = [shared_self, sign, font_family, src,
                       font_callback = std::move(font_callback),
                       response]() mutable {
            if (response.Success() && !response.data.empty()) {
              if (shared_self->CheckNodeValid(sign)) {
                font_callback(font_family, response.err_code, response.data,
                              response.data.size());
              }
              auto& font_face_cache = shared_self->GetFontFaceCache();
              font_face_cache.CacheFont(src, std::move(response.data));
              // mark other shadowNodes dirty
              const auto& it =
                  shared_self->font_family_pending_shadow_node_.find(
                      font_family);
              if (it != shared_self->font_family_pending_shadow_node_.end()) {
                for (const auto& pending_node_sign : it->second) {
                  const auto* node =
                      shared_self->node_owner_->Context()->FindShadowNodeBySign(
                          pending_node_sign);
                  if (node) {
                    node->MarkDirty();
                  }
                }
              }
              shared_self->font_family_pending_shadow_node_.erase(font_family);
            } else {
              shared_self->GetFontCollection()->SetLoadingFontState(
                  font_family,
                  FontCollectionHarmony::FontLoadingState::kLoadError);
            }
          };

          if (shared_self->node_owner_->Context()->GetLayoutTaskRunner()) {
            shared_self->node_owner_->Context()->RunOnLayoutThread(
                std::move(task));
          } else {
            shared_self->node_owner_->Context()->RunOnUIThread(std::move(task));
          }
        });
  }
}

void FontFaceManager::PrefetchFont(const std::string& src) {
  FontPrefetchCallback callback;
  PrefetchFont(src, std::move(callback));
}

void FontFaceManager::PrefetchFont(const std::string& src,
                                   FontPrefetchCallback callback) {
  if (base::DataURIUtil::IsDataURI(src)) {
    if (callback) {
      callback(0, "");
    }
    return;
  }
  const auto& font_face_cache = GetFontFaceCache();
  std::vector<uint8_t> cached_data;
  if (font_face_cache.GetFontCache(src, cached_data)) {
    if (callback) {
      callback(0, "");
    }
    return;
  }

  auto& resource_loader = node_owner_->Context()->GetResourceLoader();
  auto request = pub::LynxResourceRequest{src, pub::LynxResourceType::kFont};
  resource_loader->LoadResource(
      request, [src, callback = std::move(callback),
                weak_self = std::weak_ptr<FontFaceManager>(shared_from_this())](
                   pub::LynxResourceResponse& response) mutable {
        auto shared_self = weak_self.lock();

        if (!shared_self) {
          if (callback) {
            callback(response.err_code, response.err_msg);
          }
          return;
        }

        auto task = [shared_self, src, response,
                     callback = std::move(callback)]() mutable {
          if (response.Success() && !response.data.empty()) {
            auto& font_face_cache = shared_self->GetFontFaceCache();
            font_face_cache.CacheFont(src, std::move(response.data));
          }
          if (callback) {
            callback(response.err_code, response.err_msg);
          }
        };

        if (shared_self->node_owner_->Context()->GetLayoutTaskRunner()) {
          shared_self->node_owner_->Context()->RunOnLayoutThread(
              std::move(task));
        } else {
          shared_self->node_owner_->Context()->RunOnUIThread(std::move(task));
        }
      });
}

bool FontFaceManager::CheckNodeValid(int sign) const {
  return node_owner_->Context()->FindShadowNodeBySign(sign);
}

void FontFaceManager::TryFetchSystemFont(napi_env env, bool force_update) {
  SystemFontManager::GetInstance().GetSystemFont(
      env,
      [weak_self = weak_from_this()](std::string font_family,
                                     std::string font_path) {
        auto shared_self = weak_self.lock();
        if (shared_self) {
          shared_self->RegisterSystemFont(std::move(font_family),
                                          std::move(font_path));
        }
      },
      force_update);
}

void FontFaceManager::RegisterSystemFont(std::string font_family,
                                         std::string font_path) {
  static constexpr const char* kDefaultFontName = "default.ttf";

  auto task = [weak_self = weak_from_this(), font_family, font_path]() {
    auto shared_self = weak_self.lock();
    if (!shared_self || font_family == shared_self->default_font_family_) {
      return;
    }
    if (font_family == kDefaultFontName) {
      shared_self->default_font_family_.clear();
      LOGI("UpdateSystemFont to default font");
    } else {
      shared_self->default_font_family_ = std::move(font_family);
      LOGI("UpdateSystemFont to font:" << shared_self->default_font_family_
                                       << ",font_path:" << font_path);
      shared_self->font_collection_->RegisterFont(
          shared_self->default_font_family_.c_str(), font_path.c_str());
    }
    shared_self->node_owner_->NotifySystemFontUpdated();
  };

  if (node_owner_->Context()->GetLayoutTaskRunner()) {
    node_owner_->Context()->RunOnLayoutThread(std::move(task));
  } else {
    // it must be running in UI thread
    task();
  }
}

void FontFaceManager::TryMarkDirtyOnLayoutThread(int sign) {
  auto task = ([weak_self = weak_from_this(), sign]() {
    auto shared_self = weak_self.lock();
    if (!shared_self) {
      return;
    }
    auto it = shared_self->loading_shadow_nodes_.find(sign);

    if (it != shared_self->loading_shadow_nodes_.end()) {
      if (auto* node =
              shared_self->node_owner_->Context()->FindShadowNodeBySign(sign)) {
        node->MarkDirty();
      }
      shared_self->loading_shadow_nodes_.erase(it);
    }
  });
  if (layout_task_runner_) {
    fml::TaskRunner::RunNowOrPostTask(layout_task_runner_, std::move(task));
  } else {
    task();
  }
}

std::vector<std::string> FontFaceManager::GetCustomFamiliesFromRawString(
    const std::string& family) {
  std::vector<std::string> raw_font_families;
  base::SplitString(family, ',', raw_font_families);

  return GetCustomFamiliesFromRawVector(raw_font_families);
}

std::vector<std::string> FontFaceManager::GetCustomFamiliesFromRawVector(
    const std::vector<std::string>& raw_family_vec) {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  std::vector<std::string> custom_families;
  for (auto& raw_font_family : raw_family_vec) {
    auto it = font_faces_.find(raw_font_family);
    if (it != font_faces_.end()) {
      auto src_data_vec = it->second.GetSrcData();
      for (auto& src_data : src_data_vec) {
        custom_families.push_back(src_data.unique_custom_font_family);
      }
    } else {
      // no font-face found, use raw font family
      custom_families.push_back(raw_font_family);
    }
  }
  return custom_families;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
