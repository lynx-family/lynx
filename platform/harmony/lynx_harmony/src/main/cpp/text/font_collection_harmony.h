// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_FONT_COLLECTION_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_FONT_COLLECTION_HARMONY_H_

#include <native_drawing/drawing_font_collection.h>
#include <native_drawing/drawing_register_font.h>

#include <string>
#include <unordered_map>

#include "base/include/log/logging.h"

namespace lynx {
namespace tasm {
namespace harmony {
class FontCollectionHarmony {
 public:
  FontCollectionHarmony()
      : font_collection_(OH_Drawing_CreateSharedFontCollection()) {}

  ~FontCollectionHarmony() {
    if (font_collection_ != nullptr) {
      OH_Drawing_DestroyFontCollection(font_collection_);
    }
  }

 public:
  OH_Drawing_FontCollection* GetRawStruct() const { return font_collection_; }

  enum class FontLoadingState { kUndefined, kLoading, kLoaded, kLoadError };
  void SetLoadingFontState(const std::string& font_family,
                           FontLoadingState state) {
    font_registered_map_[font_family] = state;
  }

  FontLoadingState GetFontLoadingState(const std::string& font_family) const {
    const auto& it = font_registered_map_.find(font_family);
    if (it != font_registered_map_.end()) {
      return it->second;
    }
    return FontLoadingState::kUndefined;
  }

  void RegisterFont(const char* font_family, const char* font_path) const {
    uint32_t ret =
        OH_Drawing_RegisterFont(font_collection_, font_family, font_path);
    LOGI("RegisterFont font_family:" << font_family << ",ret:" << ret);
  }

  bool RegisterFontBuffer(const char* font_family, uint8_t* data,
                          size_t length) const {
    uint32_t ret = OH_Drawing_RegisterFontBuffer(font_collection_, font_family,
                                                 data, length);
    LOGE("RegisterFontBuffer font_family: " << font_family << ", ret: " << ret);

    return ret == 0;
  }

 private:
  OH_Drawing_FontCollection* font_collection_;
  // FIXME(linxs): check if can resue font_collection_ for different Typography,
  // then we can save the font_collection_ to LynxContext
  std::unordered_map<std::string, FontLoadingState> font_registered_map_;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_FONT_COLLECTION_HARMONY_H_
