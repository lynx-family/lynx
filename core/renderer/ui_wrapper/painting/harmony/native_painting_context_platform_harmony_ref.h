// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_NATIVE_PAINTING_CONTEXT_PLATFORM_HARMONY_REF_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_NATIVE_PAINTING_CONTEXT_PLATFORM_HARMONY_REF_H_

#include <memory>

#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"

namespace lynx {
namespace tasm {

namespace harmony {
class UIOwner;
}

class NativePaintingCtxPlatformHarmonyRef
    : public NativePaintingCtxPlatformRef {
 public:
  explicit NativePaintingCtxPlatformHarmonyRef(
      std::unique_ptr<PlatformRendererFactory> view_factory,
      harmony::UIOwner* ui_owner);
  ~NativePaintingCtxPlatformHarmonyRef() override = default;

  void SetNeedMarkPaintEndTiming(const tasm::PipelineID& pipeline_id) override;

 private:
  harmony::UIOwner* ui_owner_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_NATIVE_PAINTING_CONTEXT_PLATFORM_HARMONY_REF_H_
