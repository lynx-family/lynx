// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_ANDROID_H_

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_context.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

namespace lynx::tasm {

// Android-specific implementation of PlatformRenderer
class PlatformRendererAndroid : public PlatformRendererImpl {
 public:
  explicit PlatformRendererAndroid(
      PlatformRendererContext* context, int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config =
          PlatformRendererInitConfig());
  explicit PlatformRendererAndroid(
      PlatformRendererContext* context, int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config =
          PlatformRendererInitConfig());
  PlatformRendererAndroid(
      PlatformRendererContext* context, int id, PlatformRendererType type,
      const base::String& tag_name, const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config =
          PlatformRendererInitConfig(),
      PlatformRendererContext::PreparationScheduler::TaskRef preparation =
          nullptr);
  // UI-thread only. Native creation and layout caching do not consume the task.
  void EnsureAndroidViewCreated();
  bool HasPendingPreparation() const { return preparation_ != nullptr; }
  static base::String PreparationTag(
      PlatformRendererType type, const base::String& tag_name,
      const PlatformRendererInitConfig& init_config);
  ~PlatformRendererAndroid() override;

 protected:
  // PlatformRendererImpl interface
  void OnUpdateDisplayList(DisplayList display_list) override;
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>& attributes) override;
  void OnAddChild(PlatformRenderer* child, int index,
                  bool should_update_ui_owner) override;
  void OnRemoveFromParent(bool should_update_ui_owner) override;
  void OnUpdateSubtreeProperties(
      const DisplayList& subtree_properties) override;

 private:
  // Android-specific context for managing native views via JNI
  PlatformRendererContext* context_;
  PlatformRendererContext::PreparationScheduler::TaskRef preparation_;
  bool android_view_created_{false};

  // Initialize the Android view
  void InitializeAndroidView(const fml::RefPtr<PropBundle>& init_data,
                             jobject preparation);
  void CleanupAndroidView();
  void ClearPreparation();
  bool ShouldCreatePlatformExtendedRenderer(
      const PlatformRendererInitConfig& init_config) const;
};

// Android-specific factory
class PlatformRendererAndroidFactory : public PlatformRendererFactory {
 public:
  explicit PlatformRendererAndroidFactory(PlatformRendererContext* context)
      : context_(context) {}
  ~PlatformRendererAndroidFactory() override = default;

  PlatformRendererContext* GetContext() const { return context_; }

  fml::RefPtr<PlatformRenderer> CreateRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config =
          PlatformRendererInitConfig()) override;

  fml::RefPtr<PlatformRenderer> CreateExtendedRenderer(
      int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config =
          PlatformRendererInitConfig()) override;

 private:
  PlatformRendererContext* context_;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_ANDROID_H_
