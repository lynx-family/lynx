// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_FONT_FONT_FACE_MANAGER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_FONT_FONT_FACE_MANAGER_H_

#include <node_api.h>

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "base/include/string/string_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/font_collection_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {
class ShadowNodeOwner;
class ShadowNode;
class FontFace {
 public:
  enum class Type { URL, LOCAL };

  explicit FontFace(std::string font_family)
      : font_family_(std::move(font_family)) {}

  void ParseAndAddSrc(const std::string& src);

  const std::vector<std::pair<Type, std::string>>& GetSrc() const {
    return src_;
  }

 private:
  std::string font_family_;
  std::vector<std::pair<Type, std::string>> src_;
};

class FontFaceCache {
 public:
  bool GetFontCache(const std::string& family,
                    std::vector<uint8_t>& out) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    const auto& it = cached_font_.find(family);
    if (it != cached_font_.end()) {
      out = it->second;
      return true;
    }
    return false;
  }

  void CacheFont(const std::string& family, std::vector<uint8_t> in) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    cached_font_.emplace(family, std::move(in));
  }

 private:
  std::unordered_map<std::string, std::vector<uint8_t>> cached_font_;
  mutable std::shared_mutex mutex_;
};

using FontResourceCallback =
    base::MoveOnlyClosure<void, const std::string&, int32_t, uint8_t*, size_t>;
class FontFaceManager : public std::enable_shared_from_this<FontFaceManager> {
 public:
  explicit FontFaceManager(ShadowNodeOwner* node_owner)
      : node_owner_(node_owner),
        font_collection_(std::make_shared<FontCollectionHarmony>()) {}
  void AddFontFace(std::string font_family, FontFace font_face) {
    font_faces_.emplace(std::move(font_family), std::move(font_face));
  }

  const std::unordered_map<std::string, FontFace>& GetFontFaces() {
    return font_faces_;
  }

  void LoadFontWithUrl(int sign, const std::string& family,
                       FontResourceCallback callback);

  FontFaceCache& GetFontFaceCache() { return font_face_cache_; }

  const std::shared_ptr<FontCollectionHarmony>& GetFontCollection() {
    return font_collection_;
  }

  void TryFetchSystemFont(napi_env env, bool force_update = false);

  const std::string& GetDefaultFontFamily() { return default_font_family_; }

  //@LayoutThread
  bool CheckNodeInvalid(int sign) const;

 private:
  void RegisterSystemFont(std::string font_family, std::string font_path);
  std::unordered_map<std::string, FontFace> font_faces_;
  FontFaceCache font_face_cache_;
  ShadowNodeOwner* node_owner_{nullptr};
  std::shared_ptr<FontCollectionHarmony> font_collection_{nullptr};
  std::string default_font_family_;
  std::unordered_map<std::string, std::vector<int>>
      font_family_pending_shadow_node_;
  std::unordered_map<int, ShadowNode*> loading_shadow_nodes_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_FONT_FONT_FACE_MANAGER_H_
