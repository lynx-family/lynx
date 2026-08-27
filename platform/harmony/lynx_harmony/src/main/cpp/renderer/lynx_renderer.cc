// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer.h"

#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_display_list_applier.h"

namespace lynx {
namespace tasm {
namespace harmony {
LynxRenderer::LynxRenderer(std::shared_ptr<LynxRendererContext> context,
                           int32_t sign)
    : context_(std::move(context)), sign_(sign) {}

LynxRenderer::~LynxRenderer() = default;

void LynxRenderer::UpdateDisplayList(DisplayList display_list) {
  display_list_ = std::move(display_list);
  if (!display_list_applier_) {
    display_list_applier_ =
        std::make_unique<LynxDisplayListApplier>(context_.get());
  }
}

void LynxRenderer::Draw(OH_Drawing_Canvas* canvas) {
  if (canvas == nullptr || !display_list_applier_ ||
      display_list_.GetContentItemsSize() == 0) {
    return;
  }
  display_list_applier_->ApplyDisplayList(display_list_, canvas);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
