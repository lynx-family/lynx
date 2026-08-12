// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_CONTEXT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_CONTEXT_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/ui_wrapper/painting/paint_image.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {

class LynxImageManager;
class LynxContext;
class UIBase;

class LynxRendererContext {
 public:
  explicit LynxRendererContext(UIOwner* ui_owner);
  ~LynxRendererContext() = default;

  UIOwner* GetUIOwner() const { return ui_owner_; }
  LynxContext* GetLynxContext() const;

  void CreateImageManager(int32_t id, const std::string& src,
                          const ImagePaintInfo& paint_info, float width,
                          float height, int32_t event_mask,
                          bool disable_default_resize);
  std::shared_ptr<LynxImageManager> GetImageManager(int32_t id) const;
  void RegisterImageManagerTarget(int32_t id, std::weak_ptr<UIBase> target);
  void UnregisterImageManagerTarget(int32_t id);

  void UpdatePlatformExtraBundle(
      int32_t id,
      fml::RefPtr<fml::RefCountedThreadSafeStorage> platform_bundle);
  fml::RefPtr<fml::RefCountedThreadSafeStorage> TakePlatformExtraBundle(
      int32_t id);
  void UpdateTextBundle(
      int32_t id, fml::RefPtr<fml::RefCountedThreadSafeStorage> text_bundle);
  void DestroyTextBundle(int32_t id);
  fml::RefPtr<fml::RefCountedThreadSafeStorage> GetTextBundle(int32_t id) const;
  void RegisterPlatformExtraBundleTarget(int32_t id,
                                         std::weak_ptr<UIBase> target);
  void UnregisterPlatformExtraBundleTarget(int32_t id);

 private:
  UIOwner* ui_owner_{nullptr};
  std::unordered_map<int32_t, std::shared_ptr<LynxImageManager>>
      image_managers_;
  std::unordered_map<int32_t, std::weak_ptr<UIBase>> image_manager_targets_;
  std::unordered_map<int32_t, fml::RefPtr<fml::RefCountedThreadSafeStorage>>
      platform_extra_bundles_;
  std::unordered_map<int32_t, fml::RefPtr<fml::RefCountedThreadSafeStorage>>
      text_bundles_;
  std::unordered_map<int32_t, std::weak_ptr<UIBase>>
      platform_extra_bundle_targets_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_CONTEXT_H_
