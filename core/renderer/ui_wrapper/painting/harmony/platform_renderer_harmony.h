// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PLATFORM_RENDERER_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PLATFORM_RENDERER_HARMONY_H_

#include <memory>

#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

namespace lynx {
namespace tasm {
namespace harmony {
class LynxRendererContext;
class UIBase;
}  // namespace harmony

class PlatformRendererHarmony : public PlatformRendererImpl {
 public:
  PlatformRendererHarmony(std::shared_ptr<harmony::LynxRendererContext> context,
                          int id, PlatformRendererType type,
                          const fml::RefPtr<PropBundle>& init_data,
                          const PlatformRendererInitConfig& init_config);
  PlatformRendererHarmony(std::shared_ptr<harmony::LynxRendererContext> context,
                          int id, const base::String& tag_name,
                          const fml::RefPtr<PropBundle>& init_data,
                          const PlatformRendererInitConfig& init_config);
  ~PlatformRendererHarmony() override;

 private:
  PlatformRendererHarmony(std::shared_ptr<harmony::LynxRendererContext> context,
                          int id, PlatformRendererType type,
                          const base::String& tag_name,
                          const fml::RefPtr<PropBundle>& init_data,
                          const PlatformRendererInitConfig& init_config);

  void OnUpdateDisplayList(DisplayList display_list) override;
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                          bool tends_to_flatten) override;
  void OnAddChild(PlatformRenderer* child, int index,
                  bool should_update_ui_owner) override;
  void OnRemoveFromParent(bool should_update_ui_owner) override;
  void OnUpdateSubtreeProperties(
      const DisplayList& subtree_properties) override;

  void InitializePlatformRenderer(const fml::RefPtr<PropBundle>& init_data);
  bool InitializeUIOwnerRenderer(const base::String& tag_name,
                                 const fml::RefPtr<PropBundle>& init_data);
  bool InitializeFragmentLayerRenderer();
  bool AttachRendererToUI(harmony::UIBase* ui);
  bool ShouldCreatePlatformExtendedRenderer(
      const PlatformRendererInitConfig& init_config) const;
  void UpdateHostLayout(const DisplayList& display_list);
  void CleanupRenderer();

  std::shared_ptr<harmony::LynxRendererContext> context_;
  std::weak_ptr<harmony::UIBase> host_;
};

class PlatformRendererHarmonyFactory : public PlatformRendererFactory {
 public:
  explicit PlatformRendererHarmonyFactory(
      std::shared_ptr<harmony::LynxRendererContext> context);
  ~PlatformRendererHarmonyFactory() override = default;

  fml::RefPtr<PlatformRenderer> CreateRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config) override;
  fml::RefPtr<PlatformRenderer> CreateExtendedRenderer(
      int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config) override;

 private:
  std::shared_ptr<harmony::LynxRendererContext> context_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PLATFORM_RENDERER_HARMONY_H_
