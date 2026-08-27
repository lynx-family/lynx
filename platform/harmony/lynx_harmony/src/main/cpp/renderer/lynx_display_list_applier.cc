// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_display_list_applier.h"

#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_round_rect.h>

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"

namespace lynx {
namespace tasm {
namespace harmony {

LynxDisplayListApplier::LynxDisplayListApplier(LynxRendererContext* context)
    : context_(context) {}

LynxDisplayListApplier::~LynxDisplayListApplier() = default;

void LynxDisplayListApplier::ApplyDisplayList(const DisplayList& display_list,
                                              OH_Drawing_Canvas* canvas) {
  if (context_ == nullptr || canvas == nullptr) {
    return;
  }
  auto lynx_context = context_->GetLynxContext();
  if (lynx_context == nullptr) {
    return;
  }

  const size_t item_count = display_list.GetContentItemsSize();
  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list.GetContentItemsData());
  if (item_count == 0 || items == nullptr) {
    return;
  }
  ProcessContentOperations(items, item_count, canvas,
                           lynx_context->ScaledDensity());
}

void LynxDisplayListApplier::ProcessContentOperations(
    const DisplayListItem* items, size_t item_count, OH_Drawing_Canvas* canvas,
    float density) {}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
