// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_NATIVE_PAINTING_CONTEXT_PLATFORM_ANDROID_REF_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_NATIVE_PAINTING_CONTEXT_PLATFORM_ANDROID_REF_H_

#include <memory>
#include <string>

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"

namespace lynx {
namespace tasm {

// TODO: No methods need overriding for now; add overrides if needed in the
// future
class NativePaintingCtxAndroidRef : public NativePaintingCtxPlatformRef {
 public:
  explicit NativePaintingCtxAndroidRef(
      std::unique_ptr<PlatformRendererFactory> view_factory);
  ~NativePaintingCtxAndroidRef() override = default;

  void GetRootViewLocationOnScreen(float location[2]) override;
  void GetScreenSize(float size[2]) override;
  void GetPlatformRendererScrollOffset(int32_t sign, float offset[2]) override;
  bool IsPlatformRendererScrollable(int32_t sign) override;
  void InvokePlatformRendererUIMethod(
      int32_t id, const std::string& method, const lepus::Value& params,
      base::MoveOnlyClosure<void, int32_t, const pub::Value&> callback)
      override;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_NATIVE_PAINTING_CONTEXT_PLATFORM_ANDROID_REF_H_
