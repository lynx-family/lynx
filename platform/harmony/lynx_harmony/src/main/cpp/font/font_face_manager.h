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
#include "base/include/vector.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/font_collection_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {
class ShadowNodeOwner;
class ShadowNode;
class FontFace {
 public:
  enum class Type { URL, LOCAL };

  struct FontSrcData {
    Type type;
    std::string src;
    std::string unique_custom_font_family;  // font-family+std::hash(src)
  };

  explicit FontFace(std::string font_family)
      : font_family_(std::move(font_family)) {}

  void ParseAndAddSrc(const std::string& src);

  const std::vector<std::pair<Type, std::string>>& GetSrc() const {
    return src_;
  }
  const std::vector<FontSrcData>& GetSrcData() const { return src_data_; }

 private:
  std::string font_family_;  // raw font_family
  std::vector<std::pair<Type, std::string>> src_;
  std::vector<FontSrcData> src_data_;
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
    base::MoveOnlyClosure<void, const std::string&, int32_t,
                          std::vector<uint8_t>&, size_t>;
using FontPrefetchCallback = base::MoveOnlyClosure<void, int32_t, std::string>;
class FontFaceManager : public std::enable_shared_from_this<FontFaceManager> {
 public:
  explicit FontFaceManager(ShadowNodeOwner* node_owner,
                           bool enable_global_font_collection = false)
      : node_owner_(node_owner),
        font_collection_(
            FontCollectionHarmony::MakeSharedFontCollectionHarmony()) {}
  void AddFontFace(std::string font_family, FontFace font_face) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    font_faces_.emplace(std::move(font_family), std::move(font_face));
  }

  bool TryGetFontSrcData(
      const std::string& font_family,
      std::vector<FontFace::FontSrcData>& out_src_data) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = font_faces_.find(font_family);
    if (it != font_faces_.end()) {
      out_src_data = it->second.GetSrcData();
      return true;
    }
    return false;
  }

  void PrefetchFont(const std::string& src);
  void PrefetchFont(const std::string& src, FontPrefetchCallback callback);

  void LoadFontWithUrl(int sign, const std::string& custom_font_family,
                       const std::string& src, const FontFace::Type type,
                       FontResourceCallback callback);

  FontFaceCache& GetFontFaceCache() { return font_face_cache_; }

  const std::shared_ptr<FontCollectionHarmony>& GetFontCollection() {
    return font_collection_;
  }
  std::vector<std::string> GetCustomFamiliesFromRawString(
      const std::string& family);
  std::vector<std::string> GetCustomFamiliesFromRawVector(
      const std::vector<std::string>& raw_family_vec);

  void SetLayoutTaskRunner(const fml::RefPtr<fml::TaskRunner>& runner) {
    layout_task_runner_ = runner;
  }

  void TryFetchSystemFont(napi_env env, bool force_update = false);

  const std::string& GetDefaultFontFamily() { return default_font_family_; }

  //@LayoutThread
  bool CheckNodeValid(int sign) const;
  void TryMarkDirtyOnLayoutThread(int sign);
  void AddLoadingShadowNodes(int sign) { loading_shadow_nodes_.emplace(sign); }

 private:
  void RegisterSystemFont(std::string font_family, std::string font_path);
  std::unordered_map<std::string, FontFace> font_faces_;
  FontFaceCache font_face_cache_;
  ShadowNodeOwner* node_owner_{nullptr};
  std::shared_ptr<FontCollectionHarmony> font_collection_{nullptr};
  fml::RefPtr<fml::TaskRunner> layout_task_runner_{nullptr};

  std::string default_font_family_;
  std::unordered_map<std::string, std::vector<int>>
      font_family_pending_shadow_node_;
  base::InlineLinearFlatSet<int, 4> loading_shadow_nodes_;
  mutable std::shared_mutex mutex_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_FONT_FONT_FACE_MANAGER_H_
