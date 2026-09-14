// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_NATIVE_PAINTING_CONTEXT_PLATFORM_HARMONY_REF_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_NATIVE_PAINTING_CONTEXT_PLATFORM_HARMONY_REF_H_

#include <memory>
#include <string>
#include <vector>

#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"

namespace lynx::tasm {

namespace harmony {
class LynxRendererContext;
}

class NativePaintingCtxPlatformHarmonyRef
    : public NativePaintingCtxPlatformRef {
 public:
  explicit NativePaintingCtxPlatformHarmonyRef(
      std::unique_ptr<PlatformRendererFactory> renderer_factory,
      std::weak_ptr<harmony::LynxRendererContext> renderer_context);
  ~NativePaintingCtxPlatformHarmonyRef() override;

  void SetNeedMarkPaintEndTiming(const tasm::PipelineID& pipeline_id) override;
  std::vector<float> GetTransformValue(
      int32_t sign, const std::vector<float>& offsets) override;
  void GetRootViewLocationOnScreen(float location[2]) override;
  void GetScreenSize(float size[2]) override;
  void GetPlatformRendererScrollOffset(int32_t sign, float offset[2]) override;
  bool IsPlatformRendererScrollable(int32_t sign) override;
  void InvokePlatformViewUIMethod(
      int32_t id, const std::string& method, const lepus::Value& params,
      base::MoveOnlyClosure<void, int32_t, const pub::Value&> callback)
      override;

 protected:
  void NotifyNodeReady(const std::vector<int32_t>& signs) override;
  void DestroyImageOnPlatformThread(int32_t image_key) override;

 private:
  std::weak_ptr<harmony::LynxRendererContext> renderer_context_;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_NATIVE_PAINTING_CONTEXT_PLATFORM_HARMONY_REF_H_
