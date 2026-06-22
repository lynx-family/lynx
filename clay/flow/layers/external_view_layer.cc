// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/external_view_layer.h"

#include <memory>
#include <utility>

#include "clay/flow/embedded_views.h"
#include "clay/fml/logging.h"

namespace clay {

ExternalViewLayer::ExternalViewLayer(const clay::ElementId& element_id,
                                     const skity::Vec2& size)
    : element_id_(element_id), size_(size) {}
void ExternalViewLayer::Preroll(PrerollContext* context) {
  if (context->compositor_state == nullptr) {
    FML_DLOG(ERROR) << "Trying to embed a platform view but the PrerollContext "
                       "does not support embedding";
    return;
  }
  ContainerLayer::Preroll(context);
  context->has_platform_view = true;
  context->paints_into_platform_view_slice = true;
  set_subtree_has_platform_view(true);
  MutatorsStack mutators;
  context->state_stack.fill(&mutators);
  std::unique_ptr<EmbeddedViewParams> params =
      std::make_unique<EmbeddedViewParams>(
          context->state_stack.transform_4x4(), size_, mutators,
          EmbeddedViewPresentationKind::kOverlayLayer);
  context->compositor_state->PrerollCompositeEmbeddedView(element_id_.view_id(),
                                                          std::move(params));
}

void ExternalViewLayer::Paint(clay::PaintContext& context) const {
  FML_DCHECK(needs_painting(context));
  if (!context.compositor_state) {
    return;
  }
  auto previous_canvas = context.canvas;
  clay::GrCanvas* canvas =
      context.compositor_state->CompositeEmbeddedView(element_id_.view_id());
  FML_DCHECK(canvas != nullptr);
  context.canvas = canvas;
  context.state_stack.set_delegate(context.canvas);
  PaintChildren(context);
  // restore
  context.canvas = previous_canvas;
  context.state_stack.set_delegate(previous_canvas);
}

}  // namespace clay
