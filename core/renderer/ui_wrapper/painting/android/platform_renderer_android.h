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
  explicit PlatformRendererAndroid(PlatformRendererContext* context, int id,
                                   PlatformRendererType type,
                                   const fml::RefPtr<PropBundle>& init_data);
  explicit PlatformRendererAndroid(PlatformRendererContext* context, int id,
                                   const base::String& tag_name,
                                   const fml::RefPtr<PropBundle>& init_data);
  PlatformRendererAndroid(PlatformRendererContext* context, int id,
                          PlatformRendererType type,
                          const base::String& tag_name,
                          const fml::RefPtr<PropBundle>& init_data);
  ~PlatformRendererAndroid() override;

 protected:
  // PlatformRendererImpl interface
  void OnUpdateDisplayList(DisplayList display_list) override;
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                          bool tends_to_flatten) override;
  void OnAddChild(PlatformRenderer* child) override;
  void OnRemoveFromParent() override;

 private:
  // Android-specific context for managing native views via JNI
  PlatformRendererContext* context_;

  bool is_platform_extended_renderer_ = false;

  // Initialize the Android view
  void InitializeAndroidView(const fml::RefPtr<PropBundle>& init_data);

  // Clean up Android resources
  void CleanupAndroidView();

  // Get the display list
};

// Android-specific factory
class PlatformRendererAndroidFactory : public PlatformRendererFactory {
 public:
  explicit PlatformRendererAndroidFactory(PlatformRendererContext* context)
      : context_(context) {}
  ~PlatformRendererAndroidFactory() override = default;

  fml::RefPtr<PlatformRenderer> CreateRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data) override;

  fml::RefPtr<PlatformRenderer> CreateExtendedRenderer(
      int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>& init_data) override;

 private:
  PlatformRendererContext* context_;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_ANDROID_H_
