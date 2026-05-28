// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_FONT_COLLECTION_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_FONT_COLLECTION_HARMONY_H_

#include <native_drawing/drawing_font_collection.h>
#include <native_drawing/drawing_register_font.h>

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "base/include/log/logging.h"
#include "core/base/harmony/harmony_function_loader.h"
#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace tasm {
namespace harmony {

using FontRegisterCallback = base::MoveOnlyClosure<void>;

class FontCollectionHarmony
    : public std::enable_shared_from_this<FontCollectionHarmony> {
 public:
  FontCollectionHarmony(bool enable_system_global_collection) {
    if (enable_system_global_collection) {
      font_collection_ = GetGlobalFontCollection();
      enable_system_global_collection_ = true;
    }
    if (!font_collection_) {
      enable_system_global_collection_ = false;
      font_collection_ = OH_Drawing_CreateSharedFontCollection();
    }
  }

  ~FontCollectionHarmony() {
    if (!enable_system_global_collection_ && font_collection_ != nullptr) {
      // system global font collection shall never be destroyed!
      OH_Drawing_DestroyFontCollection(font_collection_);
    }
  }

 public:
  OH_Drawing_FontCollection* GetRawStruct() const { return font_collection_; }

  static std::shared_ptr<FontCollectionHarmony>
  MakeSharedFontCollectionHarmony() {
    static std::shared_ptr<FontCollectionHarmony>
        gGlobalFontCollectionHarmony_ =
            std::make_shared<FontCollectionHarmony>(true);
    return gGlobalFontCollectionHarmony_;
  }

  static OH_Drawing_FontCollection* GetGlobalFontCollection() {
    static OH_Drawing_FontCollection* global_font_collection = nullptr;
    static std::once_flag once;
    std::call_once(once, [&]() {
      base::harmony::NativeDrawingFunctionsHandler* handle =
          base::harmony::GetNativeDrawingFunctionsHandler();
      if (handle == nullptr) {
        LOGE("GetNativeDrawingFunctionsHandler failed");
        return;
      }

      OH_Drawing_FontCollection* collection =
          handle->oh_drawing_get_font_collection_global_instance();
      if (collection == nullptr) {
        LOGE("oh_drawing_get_font_collection_global_instance failed");
        return;
      }
      global_font_collection = collection;
    });
    return global_font_collection;
  }

  enum class FontLoadingState { kUndefined, kLoading, kLoaded, kLoadError };
  void SetLoadingFontState(const std::string& font_family,
                           FontLoadingState state) {
    std::unique_lock<std::shared_mutex> lock(font_map_mutex_);
    font_registered_map_[font_family] = state;
  }

  FontLoadingState GetFontLoadingState(const std::string& font_family) const {
    std::shared_lock<std::shared_mutex> lock(font_map_mutex_);
    const auto& it = font_registered_map_.find(font_family);
    if (it != font_registered_map_.end()) {
      return it->second;
    }
    return FontLoadingState::kUndefined;
  }

  bool IsOnUIThead() const {
    return base::UIThread::GetRunner()->GetLoop() ==
           fml::MessageLoop::GetCurrent().GetLoopImpl();
  }

  void RegisterFont(const char* font_family, const char* font_path) {
    if (IsOnUIThead()) {
      uint32_t ret =
          OH_Drawing_RegisterFont(font_collection_, font_family, font_path);
      LOGE("RegisterFontBuffer font_family: " << font_family
                                              << ", ret: " << ret);
    } else {
      std::string family(font_family);
      std::string path(font_path);
      std::weak_ptr<const FontCollectionHarmony> weak_self = shared_from_this();
      base::UIThread::GetRunner()->PostTask([weak_self, family, path] {
        if (auto self = weak_self.lock()) {
          uint32_t ret = OH_Drawing_RegisterFont(self->font_collection_,
                                                 family.c_str(), path.c_str());
          LOGE("RegisterFontBuffer font_family: " << family
                                                  << ", ret: " << ret);
        }
      });
    }
  }

  bool RegisterFontBuffer(const char* font_family, std::vector<uint8_t>& data,
                          size_t length, FontRegisterCallback callback) {
    if (IsOnUIThead()) {
      uint32_t ret = OH_Drawing_RegisterFontBuffer(
          font_collection_, font_family, data.data(), length);
      callback();
      LOGI(" RegisterFontBuffer font_family: "
           << font_family << ", ret: " << ret << ",this:" << this
           << ",font_collection:" << font_collection_);
    } else {
      std::string family(font_family);
      std::weak_ptr<const FontCollectionHarmony> weak_self = shared_from_this();
      base::UIThread::GetRunner()->PostTask(
          [weak_self, family, copy_data = data, length,
           callback = std::move(callback)]() mutable {
            if (auto self = weak_self.lock()) {
              uint32_t ret = OH_Drawing_RegisterFontBuffer(
                  self->font_collection_, family.c_str(), copy_data.data(),
                  length);
              callback();
              LOGI("RegisterFontBuffer font_family: " << family
                                                      << ", ret: " << ret);
            }
          });
    }
    return true;
  }

 private:
  OH_Drawing_FontCollection* font_collection_{nullptr};
  bool enable_system_global_collection_{false};
  std::unordered_map<std::string, FontLoadingState> font_registered_map_;
  mutable std::shared_mutex font_map_mutex_;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_FONT_COLLECTION_HARMONY_H_
