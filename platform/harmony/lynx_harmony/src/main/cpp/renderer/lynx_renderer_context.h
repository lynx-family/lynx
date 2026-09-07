// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_CONTEXT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_CONTEXT_H_

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "base/include/fml/memory/ref_ptr.h"
#include "core/renderer/ui_wrapper/painting/paint_image.h"

namespace lynx {
namespace tasm {
namespace harmony {
class LynxContext;
class LynxImageManager;
class ParagraphHarmony;
class UIBase;
class UIOwner;

class LynxRendererContext {
 public:
  explicit LynxRendererContext(std::weak_ptr<LynxContext> context);
  ~LynxRendererContext();

  std::shared_ptr<LynxContext> GetLynxContext() const;
  UIOwner* GetUIOwner() const;

  void CreateImageManager(int32_t id, const std::string& src,
                          const ImagePaintInfo& paint_info, float width,
                          float height, int32_t event_mask, int32_t image_key);
  std::shared_ptr<LynxImageManager> GetImageManager(int32_t image_key) const;
  void DestroyImageManager(int32_t image_key);

  void UpdateTextBundle(int32_t id, fml::RefPtr<ParagraphHarmony> text_bundle);
  void DestroyTextBundle(int32_t id);
  fml::RefPtr<ParagraphHarmony> GetTextBundle(int32_t id) const;

 private:
  std::weak_ptr<LynxContext> context_;
  std::unordered_map<int32_t, std::shared_ptr<LynxImageManager>>
      image_managers_;
  mutable std::mutex text_bundles_mutex_;
  std::unordered_map<int32_t, fml::RefPtr<ParagraphHarmony>> text_bundles_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_CONTEXT_H_
