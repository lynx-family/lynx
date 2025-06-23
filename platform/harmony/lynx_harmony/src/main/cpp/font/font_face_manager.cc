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

      src_.emplace_back(type, item.substr(start_pos, end_pos - start_pos + 1));
    }
  }
}

void FontFaceManager::LoadFontWithUrl(int sign, const std::string& font_family,
                                      FontResourceCallback callback) {
  const auto& it = font_faces_.find(font_family);
  if (it == font_faces_.end()) {
    callback(font_family, -1, nullptr, 0);
    return;
  }

  const FontFace& font_face = it->second;

  const auto& font_src_vec = font_face.GetSrc();

  if (font_src_vec.empty()) {
    callback(font_family, -1, nullptr, 0);
    return;
  }

  auto font_callback = fml::MakeCopyable(
      [font_family, callback = std::move(callback)](
          const std::string&, int32_t ret_code, uint8_t* data, size_t size) {
        callback(font_family, ret_code, data, size);
      });

  for (auto& item : font_src_vec) {
    if (item.first == FontFace::Type::LOCAL) {
      // TODO(linxs:) support it later
      callback(font_family, -1, nullptr, 0);
      GetFontCollection()->SetLoadingFontState(
          font_family, FontCollectionHarmony::FontLoadingState::kLoaded);
    } else if (item.first == FontFace::Type::URL) {
      const auto& font_face_cache = GetFontFaceCache();
      std::vector<uint8_t> cached_data;
      if (!node_owner_ ||
          font_face_cache.GetFontCache(font_family, cached_data)) {
        font_callback(font_family, 0, cached_data.data(), cached_data.size());
        return;
      }

      auto& src = item.second;

      if (base::DataURIUtil::IsDataURI(src)) {
        // FIXME(linxs): to be determined later, is it necessary to post decode
        // to async thread?
        std::vector<uint8_t> font_data(0);
        size_t decoded_bas64_length =
            base::DataURIUtil::DecodeDataURI(src, [&font_data](size_t size) {
              font_data.resize(size);
              return static_cast<char*>(static_cast<void*>(font_data.data()));
            });
        font_callback(font_family, 0, font_data.data(), decoded_bas64_length);
        GetFontFaceCache().CacheFont(font_family, std::move(font_data));
        GetFontCollection()->SetLoadingFontState(
            font_family, FontCollectionHarmony::FontLoadingState::kLoaded);
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
      auto request =
          pub::LynxResourceRequest{item.second, pub::LynxResourceType::kFont};
      resource_loader->LoadResource(
          request, true,
          [sign, font_family, font_callback,
           weak_self = std::weak_ptr<FontFaceManager>(shared_from_this())](
              pub::LynxResourceResponse& response) {
            auto shared_self = weak_self.lock();

            // TODO(linxs&chenyouhui): post below logic to Layout Thread, when
            // RunOnLayoutThread supported! context->RunOnLayoutThread(xxx)
            if (!shared_self) {
              return;
            }
            if (response.Success() && !response.data.empty()) {
              if (shared_self->CheckNodeInvalid(sign)) {
                font_callback(font_family, response.err_code,
                              response.data.data(), response.data.size());
              }
              shared_self->GetFontCollection()->SetLoadingFontState(
                  font_family,
                  FontCollectionHarmony::FontLoadingState::kLoaded);
              auto& font_face_cache = shared_self->GetFontFaceCache();
              font_face_cache.CacheFont(font_family, std::move(response.data));
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
          });
    }
  }
}

bool FontFaceManager::CheckNodeInvalid(int sign) const {
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

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
