// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_ANDROID_H_

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_context.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_impl.h"

namespace lynx::tasm {

// Android-specific implementation of PlatformRenderer
class PlatformRendererAndroid : public PlatformRendererImpl {
 public:
  explicit PlatformRendererAndroid(PlatformRendererContext* context, int id,
                                   PlatformRendererType type);
  explicit PlatformRendererAndroid(PlatformRendererContext* context, int id,
                                   const base::String& tag_name);
  PlatformRendererAndroid(PlatformRendererContext* context, int id,
                          PlatformRendererType type,
                          const base::String& tag_name);
  ~PlatformRendererAndroid() override;

  const DisplayList& GetDisplayList() const { return display_list_; }

 protected:
  // PlatformRendererImpl interface
  void OnUpdateDisplayList(DisplayList display_list) override;
  void OnAddChild(PlatformRenderer* child) override;
  void OnRemoveFromParent() override;

 private:
  // Android-specific context for managing native views via JNI
  PlatformRendererContext* context_;
  DisplayList display_list_;

  PlatformRendererType type_;
  base::String tag_name_;

  // Initialize the Android view
  void InitializeAndroidView();

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
      int id, PlatformRendererType type) override;

  fml::RefPtr<PlatformRenderer> CreateExtendedRenderer(
      int id, const base::String& tag_name);

 private:
  PlatformRendererContext* context_;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_ANDROID_H_
