// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_ANDROID_H_

#include <memory>

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_context.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_impl.h"

namespace lynx::tasm {

// Android-specific implementation of PlatformRenderer
class PlatformRendererAndroid : public PlatformRendererImpl {
 public:
  explicit PlatformRendererAndroid(int id);
  ~PlatformRendererAndroid() override;

 protected:
  // PlatformRendererImpl interface
  void OnUpdateDisplayList(const DisplayList& display_list) override;
  void OnAddChild(PlatformRenderer* child) override;
  void OnRemoveFromParent() override;

 private:
  // Android-specific context for managing native views via JNI
  std::shared_ptr<PlatformRendererContext> context_;

  // Initialize the Android view
  void InitializeAndroidView();

  // Clean up Android resources
  void CleanupAndroidView();
};

// Android-specific factory
class PlatformRendererAndroidFactory : public PlatformRendererFactoryImpl {
 public:
  ~PlatformRendererAndroidFactory() override = default;

  std::unique_ptr<PlatformRenderer> CreateRenderer(int id) override;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PLATFORM_RENDERER_ANDROID_H_
